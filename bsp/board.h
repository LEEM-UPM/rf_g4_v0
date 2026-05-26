/**
 * @file    board.h
 * @brief   Board-level hardware initialisation and EXTI dispatch.
 *
 * This is the top-level BSP entry point. It owns two responsibilities:
 *
 *   1. Peripheral initialisation — brings up all STM32 MCU peripherals
 *      (clocks, GPIO, SPI, UART, CAN) in the correct order via board_init().
 *
 *   2. EXTI dispatch — receives all GPIO interrupt callbacks from the STM32
 *      HAL and routes them to the correct handler via an internal table.
 *      Chip BSP modules (bsp_sx1262, bsp_sx1280, ...) register their handlers
 *      during initialisation through the driver's register_irq_handler hook.
 *
 * Architectural rule:
 *   This header is the only BSP file visible to layers above the BSP.
 *   It must not expose any chip-specific symbols, pin references, or
 *   STM32 HAL types. board_internal.h contains the internals of the
 *   dispatch mechanism and is private to the BSP.
 */
#ifndef BSP_BOARD_H_
#define BSP_BOARD_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialise all STM32 MCU peripherals.
 *
 * Performs HAL init, clock configuration, and peripheral init
 * (GPIO, SPI, UART, CAN) in the correct order.
 *
 * Does NOT initialise any external chip (SX1262, SX1280).
 * That responsibility belongs to the application layer, which
 * constructs the appropriate device objects after this returns.
 *
 * @return true always — HAL functions assert internally on failure.
 */
bool board_init(void);

#ifdef __cplusplus
}
#endif

#endif /* BSP_BOARD_H_ */