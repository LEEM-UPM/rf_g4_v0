#ifndef BSP_SX1262_SX1262_BOARD_H_
#define BSP_SX1262_SX1262_BOARD_H_

#include "sx126x_hal_context.h"

/**
 * @enum sx1262_device_id_t
 * @brief Identifiers for available SX1262 RF devices.
 */
typedef enum { RF_868_MHZ = 0, RF_DEVICE_COUNT } sx1262_device_id_t;

/**
 * @brief Retrieve the hardware context for a specific device.
 *
 * @param id Device identifier.
 * @return Pointer to the hardware context associated with the device,
 *         or NULL if the given id does not exist.
 */
const sx126x_hal_context_t *sx1262_board_get_context(sx1262_device_id_t id);

#endif /* BSP_SX1262_SX1262_BOARD_H_ */