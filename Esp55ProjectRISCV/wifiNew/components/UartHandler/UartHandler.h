#ifndef UARTHANDLER_H_
#define UARTHANDLER_H_

#include <stdint.h>
#include "driver/uart.h"
#include "freertos/semphr.h"

typedef void (*UartHandler_Callback_t)(uint8_t *data, int size);

esp_err_t UartHandler_Init(uart_config_t *uart_config, uart_port_t uart_num, uint8_t* rx_buf, int tx_pin, int rx_pin, int buf_size, UartHandler_Callback_t cb , uint16_t *fifoMaxSize) ;
void UartHandler_TaskInit(void);
void UartHandler_MainTask(void *arg);
void UartHandler_WriteData(uint8_t* tx_buf, uint32_t size, uint16_t millisecs);
uint8_t UartHandler_ReadData(uint8_t* rx_buf, uint32_t size, uint16_t millisecs);
#endif