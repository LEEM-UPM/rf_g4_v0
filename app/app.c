#include "main.h"
#include "gpio.h"
#include "stm32g4xx_hal_gpio.h"

extern void SystemClock_Config(void);

int main(void)
{
  HAL_Init();

  SystemClock_Config();

  MX_GPIO_Init();

  while (1)
  {

    HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
    HAL_Delay(500);

  }
}