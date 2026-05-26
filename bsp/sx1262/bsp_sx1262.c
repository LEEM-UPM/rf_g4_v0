/**
 * @file    bsp_sx1262.c
 * @brief   Board-specific configuration for the SX1262 on the RF board.
 *
 * This is the ONLY file in the project allowed to reference:
 *   - Concrete SPI handles       (hspi1)
 *   - Concrete GPIO ports/pins   (SX1262_CS_GPIO_Port, etc.)
 *   - RF switch GPIO control     (SX1262_LNA_EN_GPIO_Port)
 *   - Radio link parameters      (frequency, SF, BW, PA power)
 *
 * If the hardware changes (pin reassignment, new board revision, different
 * frequency band), this is the only file that needs to be modified.
 *
 * Nothing above the BSP layer (drivers, services, app) may include
 * this file or reference anything defined here.
 */

#include "bsp_sx1262.h"
#include "board_internal.h"
#include "gpio.h"
#include "spi.h"
#include "sx126x_hal_context_stm32.h"

/* -------------------------------------------------------------------------
 * RF switch control
 *
 * The PE4259 RF switch routes the antenna between the TX path (PA output)
 * and the RX path (LNA input). It is controlled by the SX1262_LNA_EN GPIO.
 *
 * Switch behaviour (verify against schematic before changing):
 *   LNA_EN = LOW  → RFC connected to RF1 → TX path active
 *   LNA_EN = HIGH → RFC connected to RF2 → RX path active
 *
 * These functions are private to this file. They are passed as callbacks
 * into sx1262_config_t so the driver can control the switch without knowing
 * anything about the GPIO or the switch topology.
 * ------------------------------------------------------------------------- */
static void board_rf_switch_set_tx(void) {
  HAL_GPIO_WritePin(SX1262_LNA_EN_GPIO_Port, SX1262_LNA_EN_Pin, GPIO_PIN_RESET);
}

static void board_rf_switch_set_rx(void) {
  HAL_GPIO_WritePin(SX1262_LNA_EN_GPIO_Port, SX1262_LNA_EN_Pin, GPIO_PIN_SET);
}

/* -------------------------------------------------------------------------
 * IRQ registration
 *
 * Bridge between the driver and the EXTI dispatch table in board.c.
 * The driver calls this during sx1262_init(), passing sx1262_dio1_irq_handler.
 * This function knows that handler corresponds to SX1262_DIO1_Pin.
 * board.c needs to know neither the handler nor the pin — it only dispatches.
 * ------------------------------------------------------------------------- */
static void bsp_sx1262_register_irq_handler(void (*handler)(void)) {
  board_register_exti_callback(SX1262_DIO1_Pin, (board_exti_callback_t)handler);
}

/* -------------------------------------------------------------------------
 * HAL context
 *
 * Describes the physical wiring between the STM32G473 and the SX1262.
 * Consumed by the Semtech sx126x_driver to perform SPI transactions.
 *
 * Pin assignments must match the CubeMX configuration in rf_g4_v0.ioc.
 * If a pin is reassigned in CubeMX, update the corresponding fields here.
 * ------------------------------------------------------------------------- */
static const sx126x_hal_context_t s_hal_ctx = {
    .spi = &hspi1,
    .nss = {.port = SX1262_CS_GPIO_Port, .pin = SX1262_CS_Pin},
    .reset = {.port = SX1262_RESET_GPIO_Port, .pin = SX1262_RESET_Pin},
    .busy = {.port = SX1262_BUSY_GPIO_Port, .pin = SX1262_BUSY_Pin},
    .dio1 = {.port = SX1262_DIO1_GPIO_Port, .pin = SX1262_DIO1_Pin},
};

/* -------------------------------------------------------------------------
 * Radio configuration
 *
 * These parameters define the LoRa link budget for the LEEM sounding rocket.
 * They encode decisions made by the RF team and are specific to this board
 * revision and the European 868 MHz ISM band.
 *
 * Parameter rationale:
 *
 *   frequency_hz = 868 MHz
 *     ETSI EN 300 220 sub-band, 1% duty cycle applies above 868.6 MHz.
 *     868.0 MHz sits in the unrestricted sub-band for short bursts.
 *
 *   SF7 + BW125 + CR4/5
 *     Symbol rate ≈ 5.5 kbps. Time-on-air for a 16-byte packet ≈ 28 ms.
 *     Link budget at +14 dBm with a 0 dBi antenna:
 *       EIRP = +14 dBm, sensitivity ≈ −124 dBm → 138 dB link budget.
 *     Sufficient for 10 km LOS with margin. Increase SF for longer range
 *     at the cost of data rate (SF12 gives −137 dBm sensitivity, 3× slower).
 *
 *   tx_power_dbm = 14
 *     Conservative value for bring-up without a matched antenna.
 *     The SX1262 supports up to +22 dBm; increase after RF validation.
 *
 *   sync_word = 0x12
 *     Private LoRa network identifier. 0x34 is reserved for LoRaWAN.
 *     Both ends of the link must use the same value.
 *
 *   use_dcdc = true
 *     Inductor L7 (15 µH) is populated on this board revision.
 *     DC-DC mode reduces current consumption vs LDO, important for
 *     battery-powered avionics. Set to false if L7 is not populated.
 * ------------------------------------------------------------------------- */
static const sx1262_config_t s_sx1262_config = {
    .hal = &s_hal_ctx,
    .set_tx = board_rf_switch_set_tx,
    .set_rx = board_rf_switch_set_rx,
    .register_irq_handler = bsp_sx1262_register_irq_handler,
    .radio =
        {
            .frequency_hz = 868000000U,
            .sf = SX126X_LORA_SF7,
            .bw = SX126X_LORA_BW_125,
            .cr = SX126X_LORA_CR_4_5,
            .tx_power_dbm = 14,
            .sync_word = 0x12,
            .use_dcdc = true,
        },
};

const sx1262_config_t *bsp_sx1262_get_config(void) { return &s_sx1262_config; }