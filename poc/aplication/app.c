/**
 * @file    app.c
 * @brief   CAN1 <-> SX1262 (LoRa) bridge.
 *
 * Echoes every CAN frame accepted by bsp_can1's filter out over LoRa, and
 * every LoRa packet received back out onto CAN1:
 *
 *   CAN1 RX (bsp_can1_poll_rx) --> sx1262_set_tx() --> sx1262_send_payload()
 *                                                   --> wait TX_DONE/TIMEOUT
 *                                                   --> sx1262_set_rx()
 *
 *   SX1262 RX_DONE (sx1262_get_event) --> sx1262_read_received_packet()
 *                                      --> bsp_can1_send()
 *
 * Wire format for the LoRa payload (mirrors bsp_can1_frame_t):
 *   byte 0-3: CAN identifier, big-endian uint32_t
 *   byte 4:   DLC (0-8)
 *   byte 5..: DLC data bytes
 *
 * NOTE ON DUTY CYCLE: unlike poc/sx1262/tx_rx.c, this bridge transmits
 * immediately on every CAN frame with no minimum spacing. At SF10+BW125
 * (~330 ms time-on-air per packet, see bsp_sx1262.c) sustained CAN traffic
 * above roughly one frame every 3.3 s will exceed the 10% duty cycle limit
 * of the 869.4-869.65 MHz sub-band. No rate limiting is applied here —
 * add one at the CAN1 polling site below if the bridge needs to run
 * unattended against real bus traffic.
 */
#include "board.h"
#include "gpio.h"
#include "main.h"
#include "spi.h"
#include "usart.h"
#include <stdio.h>
#include <string.h>
#include "sx1262.h"
#include "bsp_sx1262.h"
#include "bsp_can1.h"

/** LoRa wire-format header size (ID + DLC) before the CAN data bytes. */
#define APP_LORA_HEADER_LEN 5U

int __io_putchar(int ch) {
  HAL_UART_Transmit(&huart3, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
  return ch;
}

/** Software watchdog for app_wait_tx_done(), in milliseconds. */
#define APP_TX_WAIT_TIMEOUT_MS 1000U

/**
 * @brief Wait for the in-flight transmission to finish, with a 1 s cap.
 *
 * Blocking by design: the bridge is single-threaded and the RF switch/
 * DIO1 IRQ mask are only valid for one direction (TX or RX) at a time,
 * so CAN->LoRa forwarding cannot overlap with listening for the next
 * LoRa packet.
 *
 * sx1262_send_payload() always issues SetTx with timeout=0 (see its
 * implementation), which *disables* the chip's own TX timeout — so
 * SX1262_EVENT_TIMEOUT is not expected to fire here in practice. This
 * software deadline is the real safety net: without it, a chip lockup
 * (e.g. stuck in sx126x_hal_wait_on_busy(), as seen during bring-up)
 * would hang this loop, and with it the whole bridge, forever.
 *
 * @return true  on SX1262_EVENT_TX_DONE.
 * @return false on SX1262_EVENT_TIMEOUT, or if 1 s passes with neither
 *               event occurring (chip presumed stuck).
 */
static bool app_wait_tx_done(void) {
  uint32_t deadline = HAL_GetTick() + APP_TX_WAIT_TIMEOUT_MS;

  sx1262_irq_event_t event;
  do {
    event = sx1262_get_event();
    if (event == SX1262_EVENT_TX_DONE) {
      return true;
    }
    if (event == SX1262_EVENT_TIMEOUT) {
      return false;
    }
  } while (HAL_GetTick() < deadline);

  printf("[CAN->RF] ERROR: sin respuesta del chip tras %lu ms (posible "
         "cuelgue)\r\n",
         (uint32_t)APP_TX_WAIT_TIMEOUT_MS);
  return false;
}

/**
 * @brief Encode a CAN frame into the LoRa wire format and transmit it.
 */
static void app_forward_can_to_rf(const bsp_can1_frame_t *frame) {
  uint8_t payload[APP_LORA_HEADER_LEN + BSP_CAN1_MAX_DLC];

  payload[0] = (uint8_t)(frame->id >> 24);
  payload[1] = (uint8_t)(frame->id >> 16);
  payload[2] = (uint8_t)(frame->id >> 8);
  payload[3] = (uint8_t)(frame->id);
  payload[4] = frame->dlc;
  memcpy(&payload[APP_LORA_HEADER_LEN], frame->data, frame->dlc);

  uint8_t length = (uint8_t)(APP_LORA_HEADER_LEN + frame->dlc);

  sx1262_set_tx();
  sx1262_send_payload(payload, length);

  if (app_wait_tx_done()) {
    printf("[CAN->RF] id=0x%03lX dlc=%u OK\r\n", frame->id, frame->dlc);
  } else {
    printf("[CAN->RF] id=0x%03lX dlc=%u ERROR: timeout\r\n", frame->id,
           frame->dlc);
  }

  sx1262_set_rx();
}

/**
 * @brief Decode a received LoRa payload back into a CAN frame and send it.
 */
static void app_forward_rf_to_can(const uint8_t *payload, uint8_t length) {
  if (length < APP_LORA_HEADER_LEN) {
    printf("[RF->CAN] paquete demasiado corto (len=%u)\r\n", length);
    return;
  }

  uint32_t id = ((uint32_t)payload[0] << 24) | ((uint32_t)payload[1] << 16) |
                ((uint32_t)payload[2] << 8) | (uint32_t)payload[3];
  uint8_t dlc = payload[4];

  if (dlc > BSP_CAN1_MAX_DLC || (uint8_t)(APP_LORA_HEADER_LEN + dlc) > length) {
    printf("[RF->CAN] id=0x%03lX dlc=%u ERROR: paquete malformado\r\n", id,
           dlc);
    return;
  }

  if (bsp_can1_send(id, &payload[APP_LORA_HEADER_LEN], dlc)) {
    printf("[RF->CAN] id=0x%03lX dlc=%u OK\r\n", id, dlc);
  } else {
    printf("[RF->CAN] id=0x%03lX dlc=%u ERROR: TX FIFO llena\r\n", id, dlc);
  }
}

int main(void) {
  board_init();
  bsp_can1_start();

  const sx1262_config_t *sx1262_cfg = bsp_sx1262_get_config();
  sx1262_init(sx1262_cfg);
  sx1262_chip_config();
  sx1262_radio_config();

  printf("\r\n=== Puente CAN1 <-> SX1262 (LoRa) ===\r\n");

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

  sx1262_set_rx();

  while (1) {
    /* CAN1 -> LoRa: polled, non-blocking check first. */
    bsp_can1_frame_t can_frame;
    if (bsp_can1_poll_rx(&can_frame)) {
      app_forward_can_to_rf(&can_frame);
      HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_4);
      continue; /* re-check CAN before spending time on a LoRa event. */
    }

    /* LoRa -> CAN1: event-driven via DIO1, as in poc/sx1262/tx_rx.c. */
    sx1262_irq_event_t event = sx1262_get_event();

    switch (event) {

    case SX1262_EVENT_RX_DONE: {
      /* Toggle on every radio reception, independent of whether the
       * payload was successfully read back over SPI below. */
      HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_4);

      uint8_t rx_buf[SX1262_PAYLOAD_MAX_LEN];
      uint8_t rx_len = 0;
      if (sx1262_read_received_packet(rx_buf, &rx_len)) {
        app_forward_rf_to_can(rx_buf, rx_len);
      }
      sx1262_set_rx();
      break;
    }

    case SX1262_EVENT_CRC_ERROR:
      printf("[RF->CAN] Paquete descartado: CRC error\r\n");
      sx1262_set_rx();
      break;

    case SX1262_EVENT_TIMEOUT:
      sx1262_set_rx();
      break;

    case SX1262_EVENT_TX_DONE:
      /* Normally consumed synchronously by app_wait_tx_done(); re-arm
       * defensively if one slips through. */
      sx1262_set_rx();
      break;

    case SX1262_EVENT_NONE:
    default:
      break;
    }
  }
}
