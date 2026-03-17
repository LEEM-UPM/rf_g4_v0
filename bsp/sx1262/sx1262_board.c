#include "sx1262_board.h"
#include "gpio.h"
#include "spi.h"

/**
 * @brief Table of physical board instances.
 *
 * Each entry maps a logical identifier to its corresponding hardware.
 */
static const sx126x_hal_context_t rf_devices[RF_DEVICE_COUNT] = {
    [RF_868_MHZ] =
        {
            .spi = &hspi1,
            .nss = {.port = SX1262_CS_GPIO_Port, .pin = SX1262_CS_Pin},
            .reset = {.port = SX1262_RESET_GPIO_Port, .pin = SX1262_RESET_Pin},
            .irq = {.port = SX1262_DIO1_GPIO_Port, .pin = SX1262_DIO1_Pin},
            .busy = {.port = SX1262_BUSY_GPIO_Port, .pin = SX1262_BUSY_Pin},
        },
};

const sx126x_hal_context_t *sx1262_board_get_context(sx1262_device_id_t id) {
  if (id >= RF_DEVICE_COUNT) {
    return NULL;
  }
  return &rf_devices[id];
}