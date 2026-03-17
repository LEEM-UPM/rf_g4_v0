#include "sx1262.h"
#include "spi.h"
#include "sx1262_board.h"
#include "sx126x.h"

static const sx126x_hal_context_t *sx1262_context = NULL;

/**
 * @brief Ensure the hardware context is initialized.
 */
static void ensure_ctx(void) {
  if (sx1262_context == NULL) {
    sx1262_context = sx1262_board_get_context(RF_868_MHZ);
  }
}

/**
 * @brief Compute LoRa Low Data Rate Optimization (LDRO).
 *
 * @param sf Spreading factor.
 * @param bw Bandwidth.
 * @return 1 if LDRO should be enabled, 0 otherwise.
 */
inline static uint8_t compute_lora_ldro(const sx126x_lora_sf_t sf,
                                        const sx126x_lora_bw_t bw) {
  switch (bw) {
  case SX126X_LORA_BW_500:
    return 0;

  case SX126X_LORA_BW_250:
    if (sf == SX126X_LORA_SF12) {
      return 1;
    } else {
      return 0;
    }

  case SX126X_LORA_BW_125:
    if ((sf == SX126X_LORA_SF12) || (sf == SX126X_LORA_SF11)) {
      return 1;
    } else {
      return 0;
    }

  case SX126X_LORA_BW_062:
    if ((sf == SX126X_LORA_SF12) || (sf == SX126X_LORA_SF11) ||
        (sf == SX126X_LORA_SF10)) {
      return 1;
    } else {
      return 0;
    }

  case SX126X_LORA_BW_041:
    if ((sf == SX126X_LORA_SF12) || (sf == SX126X_LORA_SF11) ||
        (sf == SX126X_LORA_SF10) || (sf == SX126X_LORA_SF9)) {
      return 1;
    } else {
      return 0;
    }

  case SX126X_LORA_BW_031:
  case SX126X_LORA_BW_020:
  case SX126X_LORA_BW_015:
  case SX126X_LORA_BW_010:
  case SX126X_LORA_BW_007:
    return 1;

  default:
    return 0;
  }
}

void sx1262_chip_config(void) {
  ensure_ctx();
  // 1. Perform a physical reset to clear any previous state
  sx126x_reset(sx1262_context);

  // 2. Force STDBY_RC (the only mode where regulator can be changed)
  sx126x_set_standby(sx1262_context, SX126X_STANDBY_CFG_RC);

  // 3. Enable DC-DC (15uH inductor present and we are in STDBY_RC)// 3. Activar DC-DC (Tenemos L7 de 15uH y estamos en STDBY_RC)
  sx126x_set_reg_mode(sx1262_context, SX126X_REG_MODE_DCDC);

  /* Oscillator configuration: TCXO NOT USED
     We do not call set_dio3_as_tcxo.
     Internal capacitor trimming is set to minimum (0x00),
     since external capacitors (C37/C38, 10pF) are used.
     Crystal (XTAL) configuration:
     Optionally switch briefly to STDBY_XOSC so the chip
     does not overwrite our values later. */
  // sx126x_set_standby(sx1262_context, SX126X_STANDBY_CFG_XOSC);
  // sx126x_set_trimming_capacitor_values(sx1262_context, 0x00, 0x00);

  // 4. Disable automatic DIO2 control
  // Board uses manual GPIO control from STM32
  sx126x_set_dio2_as_rf_sw_ctrl(sx1262_context, false);

  // 5. Calibrate the entire system under DC-DC conditions
  sx126x_cal(sx1262_context, SX126X_CAL_ALL);
}

void sx1262_radio_config(void) {
  ensure_ctx();
  const sx126x_mod_params_lora_t sx1262_lora_mod_params = {
      .sf = SX126X_LORA_SF7,
      .bw = SX126X_LORA_BW_125,
      .cr = SX126X_LORA_CR_4_5,
      .ldro = compute_lora_ldro(SX126X_LORA_SF7, SX126X_LORA_BW_125),
  };
  const sx126x_pkt_params_lora_t sx1262_pkt_params_lora = {
      .preamble_len_in_symb = 8,
      .header_type = SX126X_LORA_PKT_EXPLICIT,
      .pld_len_in_bytes = 7,
      .crc_is_on = false,
      .invert_iq_is_on = false,
  };
  const sx126x_pa_cfg_params_t
      pa_config = // The parameters depend on the chip being used
      {
          .pa_duty_cycle = 0x03,
          .hp_max = 0x02, // 0x07 (max) = +22 dBm
          .device_sel = 0x00,
          .pa_lut = 0x01,
      };

  sx126x_set_standby(sx1262_context, SX126X_STANDBY_CFG_RC);

  // 1. Image calibration specific for 868 MHz
  // According to table 9-2: Freq1=0xD7, Freq2=0xDB
  sx126x_cal_img(sx1262_context, 0xD7, 0xDB);

  sx126x_set_pkt_type(sx1262_context, SX126X_PKT_TYPE_LORA);
  sx126x_set_rf_freq(sx1262_context, 868000000U);
  sx126x_set_pa_cfg(sx1262_context, &pa_config);

  sx126x_set_tx_params(sx1262_context, 14,
                       SX126X_RAMP_40_US); // range [-17, +22] for sub-G, range
                                           // [-18, 13] for 2.4G ( HF_PA )

  sx126x_set_rx_tx_fallback_mode(sx1262_context, SX126X_FALLBACK_STDBY_XOSC);
  sx126x_cfg_rx_boosted(sx1262_context, false);

  sx126x_set_lora_mod_params(sx1262_context, &sx1262_lora_mod_params);
  sx126x_set_lora_pkt_params(sx1262_context, &sx1262_pkt_params_lora);
  sx126x_set_lora_sync_word(sx1262_context, 0x12);

  sx126x_set_dio_irq_params(sx1262_context,
                            SX126X_IRQ_RX_DONE | SX126X_IRQ_TIMEOUT |
                                SX126X_IRQ_CRC_ERROR,
                            SX126X_IRQ_RX_DONE | SX126X_IRQ_TIMEOUT |
                                SX126X_IRQ_CRC_ERROR,
                            SX126X_IRQ_NONE,
                            SX126X_IRQ_NONE
  );

  sx126x_clear_irq_status(sx1262_context, SX126X_IRQ_ALL);
}

bool sx1262_chip_status(sx1262_chip_status_t *out_status) {
  ensure_ctx();
  sx126x_chip_status_t driver_status;
  sx126x_status_t status = sx126x_get_status(sx1262_context, &driver_status);

  if (status != SX126X_STATUS_OK || driver_status.chip_mode == 0x00) {
    return false;
  }

  out_status->chip_mode = (uint8_t)driver_status.chip_mode;
  out_status->cmd_status = (uint8_t)driver_status.cmd_status;

  return true;
}

void sx1262_set_rx(void) {
  ensure_ctx();
  // Set RF switch to RX mode?
  // Enable relevant interrupts?
  // Additional configuration may be required
}

void sx1262_set_tx(void) {
  ensure_ctx();
  // Set RF switch to TX mode?
  // Enable relevant interrupts?
  // Additional configuration may be required
}

void sx1262_send_payload(uint8_t *payload, uint8_t length) {
  ensure_ctx();
  sx126x_set_buffer_base_address(sx1262_context, 0x00, 0x00);
  sx126x_write_buffer(sx1262_context, 0x00, payload, length);

  sx126x_set_dio_irq_params(sx1262_context,
                            SX126X_IRQ_TX_DONE | SX126X_IRQ_TIMEOUT,
                            SX126X_IRQ_TX_DONE | SX126X_IRQ_TIMEOUT,
                            SX126X_IRQ_NONE, SX126X_IRQ_NONE);
  sx126x_clear_irq_status(sx1262_context, SX126X_IRQ_ALL);

  sx126x_set_tx(sx1262_context, 0); // 0 = no timeout

  uint32_t t0 = HAL_GetTick();
  sx126x_irq_mask_t irq = 0;
  while (HAL_GetTick() - t0 < 2000 && irq == 0) // 2s timeout window
  {

    sx126x_get_irq_status(sx1262_context, &irq);

    if (irq & SX126X_IRQ_TX_DONE) {
      sx126x_clear_irq_status(sx1262_context, SX126X_IRQ_TX_DONE);
      sx126x_clear_irq_status(sx1262_context, SX126X_IRQ_ALL);
    }

    if (irq & SX126X_IRQ_TIMEOUT) {
      sx126x_clear_irq_status(sx1262_context, SX126X_IRQ_TIMEOUT);
      sx126x_clear_irq_status(sx1262_context, SX126X_IRQ_ALL);
    }
  }

  sx126x_set_dio_irq_params(sx1262_context,
                            SX126X_IRQ_RX_DONE | SX126X_IRQ_TIMEOUT |
                                SX126X_IRQ_CRC_ERROR,
                            SX126X_IRQ_RX_DONE | SX126X_IRQ_TIMEOUT |
                                SX126X_IRQ_CRC_ERROR,
                            SX126X_IRQ_NONE,
                            SX126X_IRQ_NONE
  );
  sx126x_clear_irq_status(sx1262_context, SX126X_IRQ_ALL);
  sx126x_set_rx(sx1262_context, 100);
}