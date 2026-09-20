/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : mbedtls.h
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
#ifndef __mbedtlsApp_H
#define __mbedtlsApp_H
#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/debug.h"

/* MQTT CONNECT TYPES */
typedef struct
{
  uint8_t type;
  uint8_t protocol_name_length[2];
  uint8_t protocolName[4];
  uint8_t protocol_level;
  uint8_t connect_flags;
  uint8_t keep_alive[2];
}MQTT_CONNECT_HEADER;


typedef struct
{
  const char* name;
  uint8_t size[2];
  uint16_t actl_size;
}MQTT_USERNAME;

typedef struct
{
  const char* name;
  uint8_t size[2];
  uint16_t actl_size;
}MQTT_PASSWORD;

typedef struct
{
  const char* name;
  uint8_t size[2];
  uint16_t actl_size;
}MQTT_CLIENT_ID;


/* MQTT PUBLISH TYPES */
typedef struct
{
	uint16_t dataRead;
	uint16_t dataRemain;
	uint16_t dataSize;
	uint8_t  writefail;
	uint8_t  readfail;
}MQTT_HANDLER_DATA_INFO;


typedef struct
{
	uint8_t 			Type;
	uint16_t      PacketId;
	uint16_t      PayloadSize;
}MQTT_PUBLISH_CFG;

/* Global variables ---------------------------------------------------------*/

void MX_MBEDTLS_INIT_HW(void);
void Mqtt_Main_Task(void * argument);
/* MBEDTLS init function */
void MX_MBEDTLS_Init(void);

#ifdef __cplusplus
}
#endif
#endif /*__mbedtlsApp_H */