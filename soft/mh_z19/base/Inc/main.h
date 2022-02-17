/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
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
#include "stm32f3xx_hal.h"

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
#define DHT_Pin GPIO_PIN_4
#define DHT_GPIO_Port GPIOA
#define LIGHT_Pin GPIO_PIN_6
#define LIGHT_GPIO_Port GPIOA
#define LED_Pin GPIO_PIN_7
#define LED_GPIO_Port GPIOA
#define A4_Pin GPIO_PIN_12
#define A4_GPIO_Port GPIOB
#define WS2812_Pin GPIO_PIN_15
#define WS2812_GPIO_Port GPIOB
#define DOT_Pin GPIO_PIN_8
#define DOT_GPIO_Port GPIOA
#define D_Pin GPIO_PIN_9
#define D_GPIO_Port GPIOA
#define E_Pin GPIO_PIN_10
#define E_GPIO_Port GPIOA
#define C_Pin GPIO_PIN_13
#define C_GPIO_Port GPIOA
#define G_Pin GPIO_PIN_14
#define G_GPIO_Port GPIOA
#define A1_Pin GPIO_PIN_15
#define A1_GPIO_Port GPIOA
#define A_Pin GPIO_PIN_3
#define A_GPIO_Port GPIOB
#define F_Pin GPIO_PIN_4
#define F_GPIO_Port GPIOB
#define A2_Pin GPIO_PIN_5
#define A2_GPIO_Port GPIOB
#define A3_Pin GPIO_PIN_6
#define A3_GPIO_Port GPIOB
#define B_Pin GPIO_PIN_7
#define B_GPIO_Port GPIOB
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
