/**
 * @file    bsp_can1.h
 * @brief   Board-specific CAN1 (FDCAN1) access for the CAN<->LoRa bridge.
 *
 * This is the ONLY module allowed to touch the raw FDCAN1 HAL handle
 * (hfdcan1) and its filter/RAM configuration. Everything above this
 * layer works with plain uint32_t IDs and byte buffers — no HAL types
 * leak out, matching the rule in board.h.
 *
 * Bit-timing (bitrate, sample point) is owned by CubeMX / MX_FDCAN1_Init()
 * in Core/Src/fdcan.c and is NOT touched here — that file is regenerated
 * from rf_g4_v0.ioc and is expected to be maintained from CubeMX directly.
 * This module only adds what CubeMX does not generate: the acceptance
 * filter for the one CAN ID this bridge cares about, and starting the
 * peripheral.
 *
 * RX is polling-based by design (no FDCAN NVIC interrupt is wired up):
 * call bsp_can1_poll_rx() from the main loop.
 */
#ifndef BSP_CAN_BSP_CAN1_H_
#define BSP_CAN_BSP_CAN1_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Max data length of a classic CAN data frame (this board uses
 *  FDCAN_FRAME_CLASSIC, not CAN-FD — see Core/Src/fdcan.c). */
#define BSP_CAN1_MAX_DLC 8U

/**
 * Acceptance filter target ID, armed by bsp_can1_start(). Placeholder/
 * arbitrary for now, chosen so the bridge has one concrete ID to carry
 * end to end while the real bus ID is still being defined. Change this
 * to the real value once known — nothing else in the bridge needs to
 * change. Public (not just internal to bsp_can1.c) so callers can send
 * test frames on the same ID this board is listening for.
 */
#define BSP_CAN1_FILTER_ID 0x100U

/**
 * @brief One classic CAN data frame, standard (11-bit) identifier only.
 */
typedef struct {
  uint32_t id;                     /**< Standard CAN identifier, 0-0x7FF. */
  uint8_t  dlc;                    /**< Data length, 0-8 bytes.           */
  uint8_t  data[BSP_CAN1_MAX_DLC]; /**< Payload, dlc bytes significant.   */
} bsp_can1_frame_t;

/**
 * @brief Arm the single-ID acceptance filter and start FDCAN1.
 *
 * Must be called once, after board_init() (which already runs
 * MX_FDCAN1_Init() and brings the peripheral out of reset, but does not
 * start it — filters must be configured before HAL_FDCAN_Start()).
 *
 * Only standard-ID frames matching BSP_CAN1_FILTER_ID (see bsp_can1.c)
 * are accepted into RxFifo0. Everything else — other standard IDs, all
 * extended-ID frames (no extended filter is configured), and remote
 * frames — is rejected by the hardware and never reaches the CPU.
 */
void bsp_can1_start(void);

/**
 * @brief Poll for one received CAN frame (non-blocking).
 *
 * Call this repeatedly from the main loop; RX is not interrupt-driven
 * on this board. Only pops one frame per call even if more are queued.
 *
 * @param[out] out_frame  Populated with the oldest pending frame on success.
 * @return true            if a frame was waiting and has been popped.
 * @return false           if out_frame is NULL, bsp_can1_start() was not
 *                          called yet, or RxFifo0 was empty.
 */
bool bsp_can1_poll_rx(bsp_can1_frame_t *out_frame);

/**
 * @brief Transmit a classic CAN data frame with a standard identifier.
 *
 * Non-blocking: queues the frame in the FDCAN1 TX FIFO and returns
 * immediately. Does not wait for the frame to actually go out on the bus.
 *
 * @param id    Standard CAN identifier, 0-0x7FF.
 * @param data  Payload bytes. May be NULL only if dlc == 0.
 * @param dlc   Number of data bytes, 0-8 (BSP_CAN1_MAX_DLC).
 * @return true  if the frame was queued in the TX FIFO.
 * @return false if dlc > BSP_CAN1_MAX_DLC, data is NULL with dlc > 0,
 *               bsp_can1_start() was not called yet, or the TX FIFO is full.
 */
bool bsp_can1_send(uint32_t id, const uint8_t *data, uint8_t dlc);

#ifdef __cplusplus
}
#endif

#endif /* BSP_CAN_BSP_CAN1_H_ */
