/**
 * @file    bsp_sx1262.c
 * @brief   Board-specific configuration for the SX1262 on the RF board.
 *
 * This is the ONLY file in the project allowed to reference:
 *   - Concrete SPI handles       (hspi1)
 *   - Concrete GPIO ports/pins   (SX1262_CS_GPIO_Port, etc.)
 *   - RF switch GPIO control     (SX1262_SW_CTRL_GPIO_Port)
 *   - External LNA enable       (SX1262_LNA_EN_GPIO_Port)
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
#include "sx126x.h"
#include "sx126x_hal_context_stm32.h"

/* -------------------------------------------------------------------------
 * RF switch control
 *
 * The PE4259 RF switch routes the antenna between the TX path (PA output,
 * RF1) and the RX path (external LNA input, RF2). This is a DIFFERENT
 * signal from the LNA's own enable pin below — the two used to be
 * (incorrectly) tied together in code onto a single GPIO; the switch's
 * CTRL pin was actually left unconnected on the board (only tapped by a
 * status LED), so the switch was never being commanded at all. CTRL is
 * now wired to its own GPIO (SX1262_SW_CTRL, PB11).
 *
 * Switch behaviour — PE4259 datasheet Table 5 (single-pin control mode,
 * VDD pin 6 tied to +3V3, only CTRL/pin 4 driven):
 *   CTRL = HIGH → RFC connected to RF1 → TX path active
 *   CTRL = LOW  → RFC connected to RF2 → RX path active
 *
 * These functions are private to this file. They are passed as callbacks
 * into sx1262_config_t so the driver can control the switch without knowing
 * anything about the GPIO or the switch topology.
 * ------------------------------------------------------------------------- */
static void board_rf_switch_set_tx(void) {
  HAL_GPIO_WritePin(SX1262_SW_CTRL_GPIO_Port, SX1262_SW_CTRL_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(SX1262_LNA_EN_GPIO_Port, SX1262_LNA_EN_Pin, GPIO_PIN_RESET);
}

static void board_rf_switch_set_rx(void) {
  HAL_GPIO_WritePin(SX1262_SW_CTRL_GPIO_Port, SX1262_SW_CTRL_Pin, GPIO_PIN_RESET);
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
 * revision and the European 869.4-869.65 MHz ISM sub-band.
 *
 * Parameter rationale:
 *
 *   frequency_hz = 869.525 MHz
 *     ETSI EN 300 220 sub-band g3 (869.40-869.65 MHz): up to +27 dBm ERP
 *     and 10% duty cycle, vs +14 dBm ERP / 1% duty cycle in the general
 *     868.0-868.6 MHz sub-band. tx_power_dbm below (+15 dBm) sits well
 *     under both caps. 869.525 MHz is the sub-band center, matching the
 *     well-known LoRaWAN RX2 channel. Sub-band is only 250 kHz wide:
 *     BW250/BW500 would not fit inside it.
 *
 *   SF10 + BW125 + CR4/5
 *     Symbol rate ≈ 122 sym/s. Time-on-air for a 16-byte packet ≈ 330 ms.
 *     Link budget at +15 dBm with a 0 dBi antenna:
 *       EIRP = +15 dBm, sensitivity ≈ −132 dBm → 147 dB link budget.
 *     Traded data rate for range/sensitivity vs SF7 (−124 dBm, ~28 ms/packet).
 *     SF12 would push sensitivity to −137 dBm at roughly 3x the time-on-air.
 *     At 330 ms time-on-air, keep the TX repetition period above 3.3 s to
 *     stay under the 10% duty cycle limit of this sub-band.
 *
 *   tx_power_dbm = 15
 *     One step up from the +14 dBm bring-up value, still conservative.
 *     The SX1262 supports up to +22 dBm here (sub-band cap is +27 dBm
 *     ERP); increase further only after RF/antenna validation. Current
 *     draw stays modest at this level (~91 mA per datasheet Table 3-6),
 *     no VBAT headroom concerns like at +22 dBm (which needs VBAT>=3.3V).
 *
 *   sync_word = 0x12
 *     Private LoRa network identifier. 0x34 is reserved for LoRaWAN.
 *     Both ends of the link must use the same value.
 *
 *   use_dcdc = false
 *     Reverted from DC-DC: enabling it (schematic shows L7, 15 uH,
 *     matching Table 5-3) made the chip unable to complete any TX at
 *     all, worse than the LDO baseline. Root cause not yet confirmed —
 *     possible bad solder joint / continuity issue on L7, since the
 *     datasheet explicitly warns that running DC-DC without a working
 *     inductor can damage the chip (see sx1262_chip_config() comment).
 *     Do not re-enable until L7 continuity is verified with a multimeter.
 * ------------------------------------------------------------------------- */
static const sx1262_config_t s_sx1262_config = {
    .hal = &s_hal_ctx,
    .set_tx = board_rf_switch_set_tx,
    .set_rx = board_rf_switch_set_rx,
    .register_irq_handler = bsp_sx1262_register_irq_handler,
    .radio =
        {
            .frequency_hz = 869525000U,
            .sf = SX126X_LORA_SF10,
            .bw = SX126X_LORA_BW_125,
            .cr = SX126X_LORA_CR_4_5,
            .tx_power_dbm = 15,
            .sync_word = 0x12,
            .use_dcdc = false,
        },
};

const sx1262_config_t *bsp_sx1262_get_config(void) { return &s_sx1262_config; }