#include "board.h"
#include "gpio.h"
#include "main.h"
#include "spi.h"
#include "stm32g473xx.h"
#include "stm32g4xx_hal_gpio.h"
#include "usart.h"
#include <stdio.h>
#include <string.h>
#include "sx1262.h"
#include "bsp_sx1262.h"

//#define POC_MODE_TX
#define POC_MODE_RX

extern volatile uint32_t s_isr_count;

int __io_putchar(int ch) {
  HAL_UART_Transmit(&huart3, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
  return ch;
}


int main(void) {
  board_init();

  const sx1262_config_t *sx1262_cfg = bsp_sx1262_get_config();
  sx1262_init(sx1262_cfg);
  sx1262_chip_config();
  sx1262_radio_config();

  printf("\r\n=== SX1262 PoC TX/RX con interrupciones ===\r\n");

  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, SET);
  HAL_Delay(1000);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, RESET);

  sx1262_chip_status_t chip_status;
  if (!sx1262_chip_status(&chip_status)) {
    printf("ERROR: chip no responde tras config\r\n");
    while (1)
      ;
  }
  printf("Chip OK — mode=0x%02X cmd=0x%02X\r\n\r\n", chip_status.chip_mode,
         chip_status.cmd_status);

#ifdef POC_MODE_TX

  printf("Modo: TRANSMISOR\r\n\r\n");

  uint32_t tx_seq = 0;
  bool waiting_tx_done = false;

  sx1262_set_tx();

  uint8_t payload[8];
  payload[0] = (tx_seq >> 24) & 0xFF;
  payload[1] = (tx_seq >> 16) & 0xFF;
  payload[2] = (tx_seq >> 8) & 0xFF;
  payload[3] = (tx_seq) & 0xFF;
  memcpy(&payload[4], "HOLA", 4);
  sx1262_send_payload(payload, sizeof(payload));
  waiting_tx_done = true;
  printf("[TX #%lu] Enviado, esperando TX_DONE...\r\n", tx_seq);

  while (1) {
    sx1262_irq_event_t event = sx1262_get_event();

    switch (event) {

    case SX1262_EVENT_TX_DONE:
      printf("[TX #%lu] TX_DONE OK\r\n", tx_seq);
      waiting_tx_done = false;
      tx_seq++;

      /* ~330 ms time-on-air at SF10+BW125 must stay under the 10% duty
       * cycle limit of the 869.4-869.65 MHz sub-band: period >= 3.3 s. */
      HAL_Delay(3500);

      payload[0] = (tx_seq >> 24) & 0xFF;
      payload[1] = (tx_seq >> 16) & 0xFF;
      payload[2] = (tx_seq >> 8) & 0xFF;
      payload[3] = (tx_seq) & 0xFF;
      sx1262_send_payload(payload, sizeof(payload));
      waiting_tx_done = true;
      printf("[TX #%lu] Enviado, esperando TX_DONE...\r\n", tx_seq);
      HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_4);
      break;

    case SX1262_EVENT_TIMEOUT:
      printf("[TX #%lu] ERROR: timeout de TX\r\n", tx_seq);
      sx1262_send_payload(payload, sizeof(payload));
      break;

    case SX1262_EVENT_NONE: {
      static uint32_t last_diag_tick = 0;
      uint32_t now = HAL_GetTick();
      if (now - last_diag_tick >= 1000) {
        last_diag_tick = now;
        sx1262_chip_status_t diag_status = {0};
        bool status_ok = sx1262_chip_status(&diag_status);
        uint16_t dev_errors = 0;
        bool errors_ok = sx1262_get_device_errors(&dev_errors);
        printf("[DIAG] isr_count=%lu chip_status=%s mode=0x%02X cmd=0x%02X "
               "dev_errors=%s 0x%04X (PA_RAMP=%d XOSC_START=%d PLL_LOCK=%d)\r\n",
               sx1262_isr_count, status_ok ? "OK" : "FAIL",
               diag_status.chip_mode, diag_status.cmd_status,
               errors_ok ? "OK" : "FAIL", dev_errors,
               (dev_errors >> 8) & 0x01, (dev_errors >> 5) & 0x01,
               (dev_errors >> 6) & 0x01);
      }
      break;
    }

    default:
      break;
    }
  }

#elif defined(POC_MODE_RX)

  printf("Modo: RECEPTOR (escucha continua)\r\n\r\n");

  uint32_t rx_count = 0;
  uint8_t rx_buf[SX1262_PAYLOAD_MAX_LEN];
  uint8_t rx_len = 0;

  sx1262_set_rx();

  while (1) {
    sx1262_irq_event_t event = sx1262_get_event();

    switch (event) {

    case SX1262_EVENT_RX_DONE:
      if (sx1262_read_received_packet(rx_buf, &rx_len)) {
        rx_count++;

        uint32_t tx_seq = ((uint32_t)rx_buf[0] << 24) |
                          ((uint32_t)rx_buf[1] << 16) |
                          ((uint32_t)rx_buf[2] << 8) | ((uint32_t)rx_buf[3]);

        sx1262_pkt_status_t pkt_status;
        if (sx1262_get_pkt_status(&pkt_status)) {
          printf(" rssi=%d dbm \"\r\nrssi_mod= %d dbm\"\r\nsnr=%d db \"\r\n", pkt_status.rssi_dbm, pkt_status.signal_rssi_dbm, pkt_status.snr_db);
        }

        printf("[RX #%lu] seq_tx=%lu len=%u texto=\"", rx_count, tx_seq,
               rx_len);
        for (uint8_t i = 4; i < rx_len; i++) {
          printf("%c", (rx_buf[i] >= 32 && rx_buf[i] < 127) ? rx_buf[i] : '.');
        }
        printf("\"\r\n");
        HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_4);
      }

      sx1262_set_rx();
      break;

    case SX1262_EVENT_CRC_ERROR:
      printf("[RX] Paquete descartado: CRC error\r\n");
      sx1262_set_rx();
      break;

    case SX1262_EVENT_TIMEOUT:
      printf("[RX] Timeout inesperado, reiniciando RX\r\n");
      sx1262_set_rx();
      break;

    case SX1262_EVENT_NONE:
      break;

    default:
      break;
    }

  }

#else
#error "Define POC_MODE_TX o POC_MODE_RX in main.c"
#endif
}
