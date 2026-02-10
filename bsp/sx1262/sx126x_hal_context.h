/*!
 * \file      sx126x_hal_context.h
 *
 * \brief     Declaration of SX126X HAL context
 */
#ifndef SX126X_HAL_CONTEXT_H_
#define SX126X_HAL_CONTEXT_H_

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
  } irq;

  struct {
    GPIO_TypeDef *port;
    uint16_t pin;
  } busy;
} sx126x_hal_context_t;

#ifdef __cplusplus
}
#endif

#endif // SX126X_HAL_CONTEXT_H_