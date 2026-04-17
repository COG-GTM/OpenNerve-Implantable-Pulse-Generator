/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    hash.h
  * @brief   This file contains all the function prototypes for
  *          the hash.c file
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
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
#ifndef __HASH_H__
#define __HASH_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

#ifdef HAL_HASH_MODULE_ENABLED
extern HASH_HandleTypeDef hhash;
#endif

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

void MX_HASH_Init(void);
/* Note: On STM32U575xx (no hardware HASH), MX_HASH_Init is a no-op */

/* USER CODE BEGIN Prototypes */

/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __HASH_H__ */

