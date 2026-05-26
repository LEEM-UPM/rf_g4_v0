#ifndef BSP_BOARD_INTERNAL_H_
#define BSP_BOARD_INTERNAL_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Tipo del callback EXTI interno al BSP.
 *
 * Recibe el pin que disparó la interrupción. Solo visible dentro de bsp/.
 */
typedef void (*board_exti_callback_t)(uint16_t gpio_pin);

/**
 * @brief Registra un callback para un pin EXTI concreto.
 *
 * Solo puede ser llamado desde dentro de bsp/. Los módulos BSP
 * (bsp_sx1262, bsp_sx1280, etc.) lo usan durante su inicialización
 * para conectar su pin físico al handler del driver correspondiente.
 *
 * @param gpio_pin  Máscara del pin HAL (ej: GPIO_PIN_5).
 * @param callback  Función a invocar. NULL para desregistrar.
 */
void board_register_exti_callback(uint16_t gpio_pin, board_exti_callback_t callback);

#ifdef __cplusplus
}
#endif

#endif /* BSP_BOARD_INTERNAL_H_ */