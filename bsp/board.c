#include "board.h"
#include "board_internal.h"
#include "gpio.h"
#include "spi.h"
#include "usart.h"
#include "fdcan.h"
#include "main.h"

extern void SystemClock_Config(void);

/* -----------------------------------------------------------------------
 * EXTI dispatch table
 *
 * Fixed capacity, no dynamic allocation. If the table is full,
 * board_register_exti_callback() silently ignores the registration —
 * add an assert here to catch overflow during development.
 *
 * pin == 0 is the empty-slot sentinel. GPIO_PIN_0 is 0x0001 in the
 * STM32 HAL, so 0 never collides with a real pin value.
 * ----------------------------------------------------------------------- */
#define BOARD_EXTI_TABLE_SIZE 8U

typedef struct {
    uint16_t              pin;
    board_exti_callback_t callback;
} exti_entry_t;

static exti_entry_t s_exti_table[BOARD_EXTI_TABLE_SIZE] = {0};

void board_register_exti_callback(uint16_t gpio_pin, board_exti_callback_t callback) {
    for (uint8_t i = 0; i < BOARD_EXTI_TABLE_SIZE; i++) {
        if (s_exti_table[i].pin == gpio_pin) {
            s_exti_table[i].callback = callback;
            return;
        }
    }
    for (uint8_t i = 0; i < BOARD_EXTI_TABLE_SIZE; i++) {
        if (s_exti_table[i].pin == 0U) {
            s_exti_table[i].pin      = gpio_pin;
            s_exti_table[i].callback = callback;
            return;
        }
    }
}

bool board_init(void) {
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_SPI1_Init();
    MX_USART3_UART_Init();
    MX_FDCAN1_Init();
    MX_FDCAN2_Init();
    return true;
}

/*
 * Single HAL entry point for all EXTI interrupts. Dispatches to the
 * registered callback for the firing pin, if any. Unknown pins are
 * silently ignored — no handler registered means no action taken.
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    for (uint8_t i = 0; i < BOARD_EXTI_TABLE_SIZE; i++) {
        if (s_exti_table[i].pin == GPIO_Pin && s_exti_table[i].callback != NULL) {
            s_exti_table[i].callback(GPIO_Pin);
            return;
        }
    }
}