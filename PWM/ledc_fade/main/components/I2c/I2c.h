#pragma once

#include "driver/i2c_master.h"

#ifdef __cplusplus
extern "C" {
#endif
/* CODE STARTS HERE */
typedef i2c_master_bus_handle_t I2c_Bus_Master;
typedef i2c_master_dev_handle_t I2c_Dev;
typedef struct
{
    uint8_t gpio_sda;
    uint8_t gpio_scl;
} I2c_Config;
#define I2c1 0
#define I2c2 1


extern esp_err_t I2c_Init(I2c_Bus_Master* BusMaster, I2c_Dev* I2cDev, uint16_t I2cAddress, uint32_t Frequency);
extern esp_err_t I2c_DeInt(I2c_Bus_Master BusMaster);
extern esp_err_t I2c_Transmit(I2c_Dev* I2cDev, uint8_t* write_buffer, size_t size, int timeoutMs);
extern esp_err_t  I2c_TransmitReceive(I2c_Dev* I2cDev,uint8_t* write_buffer, uint8_t* read_buffer, size_t size, int timeoutMs);
extern esp_err_t I2c_Receive(I2c_Dev* I2cDev, uint8_t* read_buffer, size_t size, int timeoutMs);
/* CODE ENDS HERE */
#ifdef __cplusplus
extern "C" {
#endif