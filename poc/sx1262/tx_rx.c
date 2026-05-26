#include "board.h"
#include "gpio.h"
#include "main.h"
#include "spi.h"
#include "usart.h"
#include <stdio.h>
#include <string.h>
#include "sx1262.h"
#include "bsp_sx1262.h"

#define POC_MODE_TX
//#define POC_MODE_RX

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

      HAL_Delay(2000);

      payload[0] = (tx_seq >> 24) & 0xFF;
      payload[1] = (tx_seq >> 16) & 0xFF;
      payload[2] = (tx_seq >> 8) & 0xFF;
      payload[3] = (tx_seq) & 0xFF;
      sx1262_send_payload(payload, sizeof(payload));
      waiting_tx_done = true;
      printf("[TX #%lu] Enviado, esperando TX_DONE...\r\n", tx_seq);
      break;

    case SX1262_EVENT_TIMEOUT:
      printf("[TX #%lu] ERROR: timeout de TX\r\n", tx_seq);
      sx1262_send_payload(payload, sizeof(payload));
      break;

    case SX1262_EVENT_NONE:
      break;

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

        printf("[RX #%lu] seq_tx=%lu len=%u texto=\"", rx_count, tx_seq,
               rx_len);
        for (uint8_t i = 4; i < rx_len; i++) {
          printf("%c", (rx_buf[i] >= 32 && rx_buf[i] < 127) ? rx_buf[i] : '.');
        }
        printf("\"\r\n");
      }

      sx1262_set_rx();
      break;

    case SX1262_EVENT_CRC_ERROR:
      printf("[RX] Paquete descartado: CRC error\r\n");
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
