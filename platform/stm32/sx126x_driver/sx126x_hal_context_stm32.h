/*!
 * \file      sx126x_hal_context_stm32.h
 *
 * \brief     Declaration of SX126X HAL context
 */
#ifndef PLATFORM_STM32_SX126X_DRIVER_SX126X_HAL_CONTEXT_STM32_H_
#define PLATFORM_STM32_SX126X_DRIVER_SX126X_HAL_CONTEXT_STM32_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32g4xx_hal.h"

typedef struct {
  SPI_HandleTypeDef *spi;

  struct {
    GPIO_TypeDef *port;
    uint16_t pin;
  } nss;

  struct {
    GPIO_TypeDef *port;
    uint16_t pin;
  } reset;

  struct {
    GPIO_TypeDef *port;
    uint16_t pin;
  } dio1;

  struct {
    GPIO_TypeDef *port;
    uint16_t pin;
  } busy;
} sx126x_hal_context_t;

#ifdef __cplusplus
}
#endif

#endif /* PLATFORM_STM32_SX126X_DRIVER_SX126X_HAL_CONTEXT_STM32_H_ */