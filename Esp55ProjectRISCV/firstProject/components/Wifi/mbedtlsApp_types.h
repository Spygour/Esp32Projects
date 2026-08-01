/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : mbedtls_types.h
  * Description        : This file provides code for the configuration
  *                      of the mbedtls instances.
  ******************************************************************************
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
#ifndef __mbedtlsApp_types_H
#define __mbedtlsApp_types_H
#ifdef __cplusplus
 extern "C" {
#endif

typedef struct
{
  char* payload;
  uint16_t size;
}MQTT_PAYLOAD;

#ifdef __cplusplus
}
#endif
#endif