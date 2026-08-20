/**
 * @file    bsp_can1.c
 * @brief   Board-specific CAN1 (FDCAN1) access for the CAN<->LoRa bridge.
 *
 * See bsp_can1.h for the module's responsibilities and boundaries.
 */
#include "bsp_can1.h"
#include "fdcan.h"

/* BSP_CAN1_FILTER_ID is declared in bsp_can1.h — public so app.c can reuse
 * the same value for its boot-time CAN1 self-test frame. */

void bsp_can1_start(void) {
  /*
   * Standard-ID mask filter, exact match: FilterID2 = 0x7FF forces every
   * one of the 11 ID bits to match FilterID1, so only BSP_CAN1_FILTER_ID
   * itself passes. Requires hfdcan1.Init.StdFiltersNbr >= 1 (set in
   * Core/Src/fdcan.c) — HAL_FDCAN_ConfigFilter() rejects FilterIndex 0
   * otherwise.
   */
  FDCAN_FilterTypeDef filter = {0};
  filter.IdType = FDCAN_STANDARD_ID;
  filter.FilterIndex = 0;
  filter.FilterType = FDCAN_FILTER_MASK;
  filter.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
  filter.FilterID1 = BSP_CAN1_FILTER_ID;
  filter.FilterID2 = 0x7FFU;
  HAL_FDCAN_ConfigFilter(&hfdcan1, &filter);

  /*
   * Reject everything the filter above doesn't explicitly accept: other
   * standard IDs, all extended IDs (no extended filter is configured),
   * and remote frames on both. Only BSP_CAN1_FILTER_ID data frames reach
   * RxFifo0.
   */
  HAL_FDCAN_ConfigGlobalFilter(&hfdcan1, FDCAN_REJECT, FDCAN_REJECT,
                                FDCAN_REJECT_REMOTE, FDCAN_REJECT_REMOTE);

  HAL_FDCAN_Start(&hfdcan1);
}

bool bsp_can1_poll_rx(bsp_can1_frame_t *out_frame) {
  if (out_frame == NULL) {
    return false;
  }

  if (HAL_FDCAN_GetRxFifoFillLevel(&hfdcan1, FDCAN_RX_FIFO0) == 0U) {
    return false;
  }

  FDCAN_RxHeaderTypeDef rx_header;
  uint8_t rx_data[BSP_CAN1_MAX_DLC];
  if (HAL_FDCAN_GetRxMessage(&hfdcan1, FDCAN_RX_FIFO0, &rx_header, rx_data) !=
      HAL_OK) {
    return false;
  }

  /* Classic CAN: DataLength holds the byte count directly (0-8), not one
   * of the >8 CAN-FD codes — matches Init.FrameFormat = FDCAN_FRAME_CLASSIC. */
  out_frame->id = rx_header.Identifier;
  out_frame->dlc = (uint8_t)rx_header.DataLength;
  for (uint8_t i = 0; i < out_frame->dlc; i++) {
    out_frame->data[i] = rx_data[i];
  }

  return true;
}

bool bsp_can1_send(uint32_t id, const uint8_t *data, uint8_t dlc) {
  if (dlc > BSP_CAN1_MAX_DLC || (data == NULL && dlc > 0U)) {
    return false;
  }

  FDCAN_TxHeaderTypeDef tx_header = {0};
  tx_header.Identifier = id;
  tx_header.IdType = FDCAN_STANDARD_ID;
  tx_header.TxFrameType = FDCAN_DATA_FRAME;
  tx_header.DataLength = dlc; /* classic CAN: raw byte count, see above */
  tx_header.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
  tx_header.BitRateSwitch = FDCAN_BRS_OFF;
  tx_header.FDFormat = FDCAN_CLASSIC_CAN;
  tx_header.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
  tx_header.MessageMarker = 0;

  uint8_t tx_data[BSP_CAN1_MAX_DLC] = {0};
  for (uint8_t i = 0; i < dlc; i++) {
    tx_data[i] = data[i];
  }

  return HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &tx_header, tx_data) ==
         HAL_OK;
}
