#include "main.h"
#include "gpio.h"
#include "spi.h"
#include "sx1262.h"
#include "usart.h"
#include <stdio.h>

extern void SystemClock_Config(void);

int main(void) {
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_SPI1_Init();
  MX_USART3_UART_Init();

  sx1262_chip_config();
  sx1262_radio_config();

  sx1262_chip_status_t chip_status;

  while (1) {
    if (sx1262_chip_status(&chip_status)) {
      printf("PASS — chip_mode=0x%02X | cmd_status=0x%02X\r\n",
             chip_status.chip_mode, chip_status.cmd_status);
    } else {
      printf("FAIL\r\n");
    }

    HAL_Delay(1000);
  }
}

int __io_putchar(int ch) {
  HAL_UART_Transmit(&huart3, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
  return ch;
}
