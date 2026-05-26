/**
 * @file    bsp_sx1262.h
 * @brief   Board-specific configuration provider for the SX1262 driver.
 *
 * Architectural position:
 *
 *   This file belongs to the BSP layer. It is the only place in the project
 *   where the physical wiring of the SX1262 on the RF board is encoded:
 *   which SPI peripheral, which GPIO pins, which RF switch behaviour,
 *   and which EXTI pin carries the DIO1 interrupt.
 *
 *   The rest of the codebase (drivers, services, app) must never include
 *   this header directly. The sole consumer is the application
 *   entry point, which calls bsp_sx1262_get_config() once at startup and
 *   passes the result to sx1262_init().
 *
 * Dependency rule:
 *   This header MUST NOT be included by anything in drivers/ or above.
 *   Only the application entry point is allowed to include it,
 *   and only for the purpose of obtaining the configuration pointer to
 *   pass to sx1262_init().
 */
#ifndef BSP_SX1262_BSP_SX1262_H_
#define BSP_SX1262_BSP_SX1262_H_

#include "sx1262.h"   /* for sx1262_config_t */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Return the fully populated SX1262 configuration for this board.
 *
 * All fields are initialised as static const data, so this function is
 * safe to call before the RTOS scheduler starts.
 *
 * @return Pointer to the board's sx1262_config_t. Never NULL.
 */
const sx1262_config_t *bsp_sx1262_get_config(void);

#ifdef __cplusplus
}
#endif

#endif /* BSP_SX1262_BSP_SX1262_H_ */