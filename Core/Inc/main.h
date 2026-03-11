/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32g4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define SX1262_DIO3_Pin GPIO_PIN_13
#define SX1262_DIO3_GPIO_Port GPIOC
#define SX1262_DIO2_Pin GPIO_PIN_14
#define SX1262_DIO2_GPIO_Port GPIOC
#define SX1262_DIO1_Pin GPIO_PIN_15
#define SX1262_DIO1_GPIO_Port GPIOC
#define SX1262_DIO1_EXTI_IRQn EXTI15_10_IRQn
#define SX1262_RESET_Pin GPIO_PIN_0
#define SX1262_RESET_GPIO_Port GPIOA
#define SX1262_BUSY_Pin GPIO_PIN_1
#define SX1262_BUSY_GPIO_Port GPIOA
#define SX1262_CS_Pin GPIO_PIN_2
#define SX1262_CS_GPIO_Port GPIOA
#define SX1262_LNA_EN_Pin GPIO_PIN_3
#define SX1262_LNA_EN_GPIO_Port GPIOA
#define SX1280_RESET_Pin GPIO_PIN_0
#define SX1280_RESET_GPIO_Port GPIOB
#define SX1280_CS_Pin GPIO_PIN_1
#define SX1280_CS_GPIO_Port GPIOB
#define SX1280_LNA_EN_Pin GPIO_PIN_2
#define SX1280_LNA_EN_GPIO_Port GPIOB
#define SX1280_BUSY_Pin GPIO_PIN_15
#define SX1280_BUSY_GPIO_Port GPIOB
#define SX1280_DIO1_Pin GPIO_PIN_8
#define SX1280_DIO1_GPIO_Port GPIOA
#define FDCAN1_STB_Pin GPIO_PIN_9
#define FDCAN1_STB_GPIO_Port GPIOA
#define FDCAN2_STB_Pin GPIO_PIN_10
#define FDCAN2_STB_GPIO_Port GPIOA
#define SX1280_DIO2_Pin GPIO_PIN_15
#define SX1280_DIO2_GPIO_Port GPIOA
#define SX1280_DIO3_Pin GPIO_PIN_3
#define SX1280_DIO3_GPIO_Port GPIOB
#define LED_Pin GPIO_PIN_4
#define LED_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
