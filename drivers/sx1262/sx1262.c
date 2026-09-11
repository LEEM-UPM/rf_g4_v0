/**
 * @file    sx1262.c
 * @brief   Mid-level driver for the SX1262 sub-GHz radio chip.
 *
 * This file contains zero board-specific references. No STM32 HAL headers,
 * no GPIO macros, no SPI handles. All hardware access goes through the
 * sx1262_config_t injected by the BSP at initialisation time.
 */

#include "sx1262.h"
#include "critical_section.h"
#include <stddef.h>
#include <string.h>

/**
 * A single static instance is sufficient because there is one SX1262 on
 * the RF board. If a second chip were added, this would need to become
 * an instance handle passed through every API call.
 **/

/** Configuration injected by the BSP. Valid after sx1262_init() returns true.
 */
static const sx1262_config_t *s_config = NULL;

/**
 * Pending IRQ event.
 *
 * Written from interrupt context by sx1262_dio1_irq_handler().
 * Read and cleared in task context by sx1262_get_event().
 *
 * volatile is mandatory: without it the compiler may cache the value in a
 * register and the task context would never observe the ISR's update.
 */
static volatile bool s_irq_pending = false;

/** Diagnostic counter — number of DIO1 edges seen since reset. */
volatile uint32_t sx1262_isr_count = 0;

/**
 * @brief Compute whether Low Data Rate Optimisation (LDRO) must be enabled.
 *
 * LDRO must be active when the LoRa symbol time is >= 16.38 ms.
 * Symbol time = 2^SF / BW.
 *
 * Examples:
 *   SF12 + BW125 → 32.77 ms → LDRO ON
 *   SF7  + BW125 →  1.02 ms → LDRO OFF
 *
 * Reference: datasheet §6.1.1.4 and §13.4.5.
 */
static uint8_t compute_ldro(sx126x_lora_sf_t sf, sx126x_lora_bw_t bw) {
  switch (bw) {
  case SX126X_LORA_BW_500:
    return 0;

  case SX126X_LORA_BW_250:
    return (sf == SX126X_LORA_SF12) ? 1 : 0;

  case SX126X_LORA_BW_125:
    return (sf >= SX126X_LORA_SF11) ? 1 : 0;

  case SX126X_LORA_BW_062:
    return (sf >= SX126X_LORA_SF10) ? 1 : 0;

  case SX126X_LORA_BW_041:
    return (sf >= SX126X_LORA_SF9) ? 1 : 0;

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

/**
 * @brief Route the RF switch to TX path via the BSP-supplied callback.
 * No-op if the board has no RF switch (set_tx == NULL).
 */
static void rf_switch_set_tx(void) {
  if (s_config->set_tx != NULL) {
    s_config->set_tx();
  }
}

/**
 * @brief Route the RF switch to RX path via the BSP-supplied callback.
 * No-op if the board has no RF switch (set_rx == NULL).
 */
static void rf_switch_set_rx(void) {
  if (s_config->set_rx != NULL) {
    s_config->set_rx();
  }
}

bool sx1262_init(const sx1262_config_t *config) {
  if (config == NULL)
    return false;
  if (config->hal == NULL)
    return false;

  /*
   * RF switch callbacks are optional: a board with a fixed-path antenna
   * (no external switch) may legitimately pass NULL for both. We allow
   * that. What we do not allow is a half-initialised switch (one NULL,
   * one non-NULL), which would indicate a BSP programming error.
   */
  bool tx_null = (config->set_tx == NULL);
  bool rx_null = (config->set_rx == NULL);
  if (tx_null != rx_null)
    return false; /* both or neither */

  s_config = config;

  if (config->register_irq_handler != NULL) {
    config->register_irq_handler(sx1262_dio1_irq_handler);
  }
  
  return true;
}

/**
 * @brief Hardware initialisation sequence for the SX1262.
 *
 * Step-by-step rationale:
 *
 *  1. Physical reset: clears any state left from a previous firmware run.
 *     The chip needs ~1 ms with NRESET low and ~5 ms after release before
 *     the first SPI transaction (datasheet §8.1). The HAL impl handles this.
 *
 *  2. Force STDBY_RC: the DC-DC regulator can only be switched while the
 *     chip is in STDBY_RC. Any other mode rejects SetRegulatorMode.
 *
 *  3. Regulator selection: DC-DC if the BSP says the inductor is present,
 *     LDO otherwise. Running DC-DC without the inductor damages the chip.
 *
 *  4. Disable DIO2 auto RF-switch control: this board uses a manual GPIO
 *     (PE4259 switch controlled by the STM32) instead of DIO2. Leaving
 *     this enabled would fight the manual control.
 *
 *  5. Full system calibration: must run under the final regulator settings
 *     so the calibration values are valid for actual operating conditions.
 *
 *  6. TxClamp workaround (datasheet §15.2): after reset, bits [4:1] of
 *     register 0x08D8 must be set to 0b1111. Without this, the PA clips
 *     output power when the antenna VSWR is high, losing up to 6 dBm.
 */
void sx1262_chip_config(void) {
  const void *hal = s_config->hal;

  sx126x_reset(hal);
  sx126x_set_standby(hal, SX126X_STANDBY_CFG_RC);

  sx126x_reg_mod_t regulator =
      s_config->radio.use_dcdc ? SX126X_REG_MODE_DCDC : SX126X_REG_MODE_LDO;
  sx126x_set_reg_mode(hal, regulator);

  sx126x_set_dio2_as_rf_sw_ctrl(hal, false);
  sx126x_cal(hal, SX126X_CAL_ALL);

  /*
   * TxClamp workaround (datasheet §15.2): bits [4:1] of register 0x08D8
   * must be set to 0b1111. Without this the PA over-voltage clamp is
   * overly protective and backs off output power (up to 6 dB) whenever
   * the antenna is even moderately mismatched.
   */
  uint8_t tx_clamp_cfg;
  sx126x_read_register(hal, 0x08D8, &tx_clamp_cfg, 1);
  tx_clamp_cfg |= 0x1E;
  sx126x_write_register(hal, 0x08D8, &tx_clamp_cfg, 1);
}

/**
 * @brief Configure the radio for LoRa operation.
 *
 * All parameters come from s_config->radio, which the BSP set at init time.
 * The mandatory sequencing required by datasheet §14.4 is:
 *   SetPacketType → SetRfFrequency → SetPaConfig → SetTxParams →
 *   SetModulationParams → SetPacketParams
 *
 * Image calibration is performed here (not in chip_config) because it is
 * frequency-dependent and must run after SetRfFrequency. The frequency
 * band thresholds come from datasheet table 9-2.
 *
 * Fallback mode is set to STDBY_XOSC so the oscillator stays running
 * between packets, reducing TX/RX transition latency.
 */
void sx1262_radio_config(void) {
  const void *hal = s_config->hal;
  const uint32_t freq = s_config->radio.frequency_hz;

  sx126x_set_standby(hal, SX126X_STANDBY_CFG_RC);

  /*
   * Image calibration frequency bands (datasheet table 9-2).
   * The two bytes bracket the band; the chip calibrates its image
   * rejection filter for that frequency range.
   */
  uint8_t cal_freq1, cal_freq2;
  if (freq < 446000000U) {
    cal_freq1 = 0x6B;
    cal_freq2 = 0x6F;
  } else if (freq < 734000000U) {
    cal_freq1 = 0x75;
    cal_freq2 = 0x81;
  } else if (freq < 828000000U) {
    cal_freq1 = 0xC1;
    cal_freq2 = 0xC5;
  } else if (freq < 877000000U) {
    cal_freq1 = 0xD7;
    cal_freq2 = 0xDB;
  } else if (freq < 1000000000U) {
    cal_freq1 = 0xE1;
    cal_freq2 = 0xE9;
  } else {
    cal_freq1 = 0xD7;
    cal_freq2 = 0xDB;
  }

  sx126x_cal_img(hal, cal_freq1, cal_freq2);

  /* --- Mandatory sequence: packet type first --- */
  sx126x_set_pkt_type(hal, SX126X_PKT_TYPE_LORA);
  sx126x_set_rf_freq(hal, freq);

  /*
   * PA configuration for SX1262 (not SX1261 — device_sel = 0x00).
   *
   * pa_duty_cycle/hp_max are fixed at the maximum HP-PA setting (datasheet
   * table 13-21, "+22 dBm" row). This is deliberately NOT the "PA optimal
   * settings" table, which requires pa_duty_cycle/hp_max to be re-matched
   * to tx_power_dbm every time it changes, AND SetTxParams pinned to
   * +22 dBm regardless of the actual target power. Fixing the PA at max
   * and letting SetTxParams set the real output power directly (valid
   * range -9 to +22 dBm, datasheet §13.4.4) is less efficient but far
   * less error-prone — tx_power_dbm alone controls the output level.
   */
  const sx126x_pa_cfg_params_t pa_cfg = {
      .pa_duty_cycle = 0x04,
      .hp_max = 0x07,
      .device_sel = 0x00,
      .pa_lut = 0x01,
  };
  sx126x_set_pa_cfg(hal, &pa_cfg);
  sx126x_set_tx_params(hal, s_config->radio.tx_power_dbm, SX126X_RAMP_40_US);

  /*
   * After TX or RX, fall back to STDBY_XOSC rather than STDBY_RC.
   * The oscillator stays running so the next TX/RX transition is faster.
   * Acceptable power trade-off for a telemetry link with frequent packets.
   */
  sx126x_set_rx_tx_fallback_mode(hal, SX126X_FALLBACK_STDBY_XOSC);

  /* Standard LNA gain. Boosted mode gains ~3 dB sensitivity at the cost
   * of ~2 mA extra current — not needed for a 10 km link. */
  sx126x_cfg_rx_boosted(hal, false);

  /* --- Modulation and packet parameters --- */
  const sx126x_mod_params_lora_t mod = {
      .sf = s_config->radio.sf,
      .bw = s_config->radio.bw,
      .cr = s_config->radio.cr,
      .ldro = compute_ldro(s_config->radio.sf, s_config->radio.bw),
  };
  sx126x_set_lora_mod_params(hal, &mod);

  /*
   * Modulation quality workaround (datasheet §15.1): bit 2 of register
   * 0x0889 must be 0 only for 500 kHz LoRa bandwidth, and 1 for any other
   * LoRa BW (or FSK). The reset value (0x01) has this bit cleared, which
   * is the *wrong* setting for any BW below 500 kHz and would degrade the
   * remote receiver's sensitivity without this fix.
   */
  uint8_t tx_mod_cfg;
  sx126x_read_register(hal, 0x0889, &tx_mod_cfg, 1);
  if (s_config->radio.bw == SX126X_LORA_BW_500) {
    tx_mod_cfg &= ~0x04;
  } else {
    tx_mod_cfg |= 0x04;
  }
  sx126x_write_register(hal, 0x0889, &tx_mod_cfg, 1);

  /*
   * pld_len_in_bytes is set to 0 here because sx1262_send_payload()
   * updates it with the actual payload length before every transmission.
   * In EXPLICIT header mode the receiver reads the length from the packet
   * header, so the RX side does not need to know it in advance.
   */
  const sx126x_pkt_params_lora_t pkt = {
      .preamble_len_in_symb = 8,
      .header_type = SX126X_LORA_PKT_EXPLICIT,
      .pld_len_in_bytes = 0,
      .crc_is_on = true,
      .invert_iq_is_on = false,
  };
  sx126x_set_lora_pkt_params(hal, &pkt);

  /*
   * Sync word 0x12 identifies this as a private LoRa network.
   * 0x34 is the LoRaWAN public network sync word.
   * Both sides must use the same value or packets will not be detected.
   */
  sx126x_set_lora_sync_word(hal, s_config->radio.sync_word);

  sx126x_set_dio_irq_params(hal,
                            SX126X_IRQ_RX_DONE | SX126X_IRQ_TIMEOUT |
                                SX126X_IRQ_CRC_ERROR,
                            SX126X_IRQ_RX_DONE | SX126X_IRQ_TIMEOUT |
                                SX126X_IRQ_CRC_ERROR,
                            SX126X_IRQ_NONE,
                            SX126X_IRQ_NONE
  );

  sx126x_clear_irq_status(hal, SX126X_IRQ_ALL);
}

bool sx1262_chip_status(sx1262_chip_status_t *out) {
  if (out == NULL)
    return false;

  sx126x_chip_status_t raw;
  if (sx126x_get_status(s_config->hal, &raw) != SX126X_STATUS_OK) {
    return false;
  }
  if (raw.chip_mode == 0x00) {
    return false; /* chip did not respond */
  }

  out->chip_mode = (uint8_t)raw.chip_mode;
  out->cmd_status = (uint8_t)raw.cmd_status;
  return true;
}

void sx1262_set_tx(void) {
  rf_switch_set_tx();

  sx126x_set_dio_irq_params(s_config->hal,
                            SX126X_IRQ_TX_DONE | SX126X_IRQ_TIMEOUT,
                            SX126X_IRQ_TX_DONE | SX126X_IRQ_TIMEOUT,
                            SX126X_IRQ_NONE, SX126X_IRQ_NONE);
  sx126x_clear_irq_status(s_config->hal, SX126X_IRQ_ALL);
  s_irq_pending = false;
}

bool sx1262_send_payload(const uint8_t *payload, uint8_t length) {
  if (payload == NULL || length == 0 || length > SX1262_PAYLOAD_MAX_LEN) {
    return false;
  }

  /*
   * Update the packet length in the chip before every transmission.
   * In EXPLICIT header mode the length is embedded in the packet header,
   * so the receiver does not need to know it in advance — but the
   * transmitter must set it correctly here so the chip knows how many
   * bytes to pull from the buffer.
   */
  const sx126x_pkt_params_lora_t pkt = {
      .preamble_len_in_symb = 8,
      .header_type = SX126X_LORA_PKT_EXPLICIT,
      .pld_len_in_bytes = length,
      .crc_is_on = true,
      .invert_iq_is_on = false,
  };
  sx126x_set_lora_pkt_params(s_config->hal, &pkt);

  sx126x_set_buffer_base_address(s_config->hal, 0x00, 0x00);
  sx126x_write_buffer(s_config->hal, 0x00, (uint8_t *)payload, length);

  /* Timeout = 0: the chip transmits and returns to STDBY_XOSC on its own. */
  sx126x_set_tx(s_config->hal, 0);

  return true;
}

void sx1262_set_rx(void) {
  rf_switch_set_rx();

  sx126x_set_buffer_base_address(s_config->hal, 0x00, 0x00);
  sx126x_set_dio_irq_params(
      s_config->hal,
      SX126X_IRQ_RX_DONE | SX126X_IRQ_TIMEOUT | SX126X_IRQ_CRC_ERROR,
      SX126X_IRQ_RX_DONE | SX126X_IRQ_TIMEOUT | SX126X_IRQ_CRC_ERROR,
      SX126X_IRQ_NONE, SX126X_IRQ_NONE);
  sx126x_clear_irq_status(s_config->hal, SX126X_IRQ_ALL);
  s_irq_pending = false;

  /*
   * Deliberately Single mode (timeout=0), NOT SX126X_RX_CONTINUOUS.
   *
   * This function is called again by the application after every
   * RX_DONE/TIMEOUT event (see tx_rx.c), so re-arming is already handled
   * explicitly at the app layer. In Single mode, each SetRx cleanly
   * resets RxDataPointer to RxBaseAddr (datasheet §7.2) — matching the
   * SetBufferBaseAddress(0,0) issued above on every call.
   *
   * Continuous mode was tried here, but it actively fights this design:
   * the chip auto-rearms and keeps writing the buffer from wherever the
   * internal RxDataPointer was left (it *increments* rather than resets,
   * §7.2), while this function keeps forcing the base address back to 0
   * and re-issuing SetRx on top of a receiver that may already be
   * mid-listening or mid-reception — corrupting/freezing subsequent
   * receptions instead of cleanly capturing each new packet.
   */
  sx126x_set_rx(s_config->hal, 0);
}

bool sx1262_read_received_packet(uint8_t *out_buf, uint8_t *out_length) {
  if (out_buf == NULL || out_length == NULL)
    return false;

  sx126x_rx_buffer_status_t buf_status;
  if (sx126x_get_rx_buffer_status(s_config->hal, &buf_status) !=
      SX126X_STATUS_OK) {
    return false;
  }

  *out_length = buf_status.pld_len_in_bytes;
  sx126x_read_buffer(s_config->hal, buf_status.buffer_start_pointer, out_buf,
                     *out_length);

  return true;
}

bool sx1262_get_pkt_status(sx1262_pkt_status_t *out) {
  if (out == NULL)
    return false;

  sx126x_pkt_status_lora_t raw;
  if (sx126x_get_lora_pkt_status(s_config->hal, &raw) != SX126X_STATUS_OK) {
    return false;
  }

  out->rssi_dbm = raw.rssi_pkt_in_dbm;
  out->snr_db = raw.snr_pkt_in_db;
  out->signal_rssi_dbm = raw.signal_rssi_pkt_in_dbm;
  return true;
}

bool sx1262_get_device_errors(uint16_t *out_errors) {
  if (out_errors == NULL)
    return false;

  sx126x_errors_mask_t raw;
  if (sx126x_get_device_errors(s_config->hal, &raw) != SX126X_STATUS_OK) {
    return false;
  }

  *out_errors = (uint16_t)raw;
  sx126x_clear_device_errors(s_config->hal);
  return true;
}

/**
 * Runs in interrupt context — must be as short as possible.
 *
 * We deliberately do NOT read the IRQ status register here. An SPI
 * transaction inside an ISR would block for hundreds of microseconds and
 * could starve other interrupts. Instead we set a flag and defer the SPI
 * read to sx1262_get_event(), which runs in task/main-loop context.
 */
void sx1262_dio1_irq_handler(void) {
  s_irq_pending = true;
  sx1262_isr_count++;
}

/**
 * Runs in task context — safe to perform SPI here.
 *
 * Reads the chip IRQ status register, clears the flags, translates them
 * to sx1262_irq_event_t, and resets the pending flag atomically.
 *
 * CRITICAL_ENTER/EXIT (from critical_section.h) protects the read-modify
 * of s_irq_pending against the ISR. When RTOS is introduced, this
 * entire notification mechanism should be replaced: the ISR gives a binary
 * semaphore and this function takes it, eliminating
 * the need for polling and the critical section entirely.
 */
sx1262_irq_event_t sx1262_get_event(void) {
  CRITICAL_ENTER();
  bool pending = s_irq_pending;
  s_irq_pending = false;
  CRITICAL_EXIT();

  if (!pending) {
    return SX1262_EVENT_NONE;
  }

  /* Deferred SPI read — safe because we are in task context now. */
  sx126x_irq_mask_t irq_status;
  sx126x_get_irq_status(s_config->hal, &irq_status);
  sx126x_clear_irq_status(s_config->hal, SX126X_IRQ_ALL);

  if (irq_status & SX126X_IRQ_TX_DONE)
    return SX1262_EVENT_TX_DONE;
  /*
   * CRC_ERROR must be checked before RX_DONE: on a corrupted packet the
   * chip raises both bits together (reception completed, but the CRC
   * check failed). Checking RX_DONE first would silently accept garbage
   * payloads as valid.
   */
  if (irq_status & SX126X_IRQ_CRC_ERROR)
    return SX1262_EVENT_CRC_ERROR;
  if (irq_status & SX126X_IRQ_RX_DONE)
    return SX1262_EVENT_RX_DONE;
  if (irq_status & SX126X_IRQ_TIMEOUT)
    return SX1262_EVENT_TIMEOUT;

  return SX1262_EVENT_NONE;
}