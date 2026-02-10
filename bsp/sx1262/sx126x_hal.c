/**
 * @file      sx126x_hal.c
 *
 * @brief     Implements the SX126x radio HAL functions
 */

#include <stddef.h>

#include "sx126x_hal.h"
#include "sx126x_hal_context.h"

/**
 * @brief Wait until radio busy pin returns to 0
 */
static void sx126x_hal_wait_on_busy(const void* context);

sx126x_hal_status_t sx126x_hal_reset(const void* context)
{
    const sx126x_hal_context_t* sx126x_context = (const sx126x_hal_context_t*)context;

    HAL_GPIO_WritePin(sx126x_context->reset.port, sx126x_context->reset.pin, GPIO_PIN_RESET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(sx126x_context->reset.port, sx126x_context->reset.pin, GPIO_PIN_SET);

    return SX126X_HAL_STATUS_OK;
}

sx126x_hal_status_t sx126x_hal_wakeup(const void* context)
{
    const sx126x_hal_context_t* sx126x_context = (const sx126x_hal_context_t*)context;

    HAL_GPIO_WritePin(sx126x_context->nss.port, sx126x_context->nss.pin, GPIO_PIN_RESET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(sx126x_context->nss.port, sx126x_context->nss.pin, GPIO_PIN_SET);

    return SX126X_HAL_STATUS_OK;
}

sx126x_hal_status_t sx126x_hal_write(const void* context, const uint8_t* command, const uint16_t command_length,
                                     const uint8_t* data, const uint16_t data_length)
{
    const sx126x_hal_context_t* sx126x_context = (const sx126x_hal_context_t*)context;

    sx126x_hal_wait_on_busy(sx126x_context);

    HAL_GPIO_WritePin(sx126x_context->nss.port, sx126x_context->nss.pin, GPIO_PIN_RESET);
    HAL_SPI_Transmit(sx126x_context->spi, command, command_length, HAL_MAX_DELAY);
    HAL_SPI_Transmit(sx126x_context->spi, data, data_length, HAL_MAX_DELAY); // TODO: Creo que falla si data_length = 0 (como en sx126x_set_rx())
    HAL_GPIO_WritePin(sx126x_context->nss.port, sx126x_context->nss.pin, GPIO_PIN_SET);

    return SX126X_HAL_STATUS_OK;
}

sx126x_hal_status_t sx126x_hal_read(const void* context, const uint8_t* command, const uint16_t command_length,
                                    uint8_t* data, const uint16_t data_length)
{
    const sx126x_hal_context_t* sx126x_context = (const sx126x_hal_context_t*)context;

    sx126x_hal_wait_on_busy(sx126x_context);

    HAL_GPIO_WritePin(sx126x_context->nss.port, sx126x_context->nss.pin, GPIO_PIN_RESET);
    HAL_SPI_Transmit(sx126x_context->spi, command, command_length, HAL_MAX_DELAY);
    HAL_SPI_Receive(sx126x_context->spi, data, data_length, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(sx126x_context->nss.port, sx126x_context->nss.pin, GPIO_PIN_SET);

    return SX126X_HAL_STATUS_OK;
}

static void sx126x_hal_wait_on_busy(const void* context)
{
    const sx126x_hal_context_t* sx126x_context = (const sx126x_hal_context_t*)context;
    GPIO_PinState gpio_state;

    do
    {
        gpio_state = HAL_GPIO_ReadPin(sx126x_context->busy.port, sx126x_context->busy.pin);
    } while( gpio_state == GPIO_PIN_SET);
}