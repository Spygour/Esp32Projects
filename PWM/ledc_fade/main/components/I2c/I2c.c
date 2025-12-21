#include "I2c.h"


/* Local Params */
I2c_Config I2cConfig  = 
{
    21,
    22
};


esp_err_t I2c_Init(I2c_Bus_Master* BusMaster, I2c_Dev* I2cDev, uint16_t I2cAddress, uint32_t Frequency)
{
    i2c_master_bus_config_t I2cBusCfg = 
    {
        .clk_source = SOC_MOD_CLK_APB,
        .glitch_ignore_cnt = 7,
        .sda_io_num = I2cConfig.gpio_sda,
        .scl_io_num = I2cConfig.gpio_scl,
        .intr_priority = 0,
        .i2c_port = 0,
        .trans_queue_depth = 20,
        .flags = {.enable_internal_pullup = 0}
    };

    esp_err_t I2cInitVar = i2c_new_master_bus(&I2cBusCfg, BusMaster);

    i2c_device_config_t I2cDevCfg = 
    {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = I2cAddress,
        .flags = {.disable_ack_check = 1},
        .scl_speed_hz = Frequency,
        .scl_wait_us = 100000 /* Max wait is 100 ms */
    };

    I2cInitVar = i2c_master_bus_add_device(*BusMaster, &I2cDevCfg, I2cDev);
    return !I2cInitVar;
}

esp_err_t I2c_DeInt(I2c_Bus_Master BusMaster)
{
    return !(i2c_del_master_bus(BusMaster));
}

esp_err_t I2c_Transmit(I2c_Dev* I2cDev, uint8_t* write_buffer, size_t size, int timeoutMs)
{
    return !(i2c_master_transmit(*I2cDev, write_buffer, size, timeoutMs));
}

esp_err_t  I2c_TransmitReceive(I2c_Dev* I2cDev,uint8_t* write_buffer, uint8_t* read_buffer, size_t size, int timeoutMs)
{
    return !(i2c_master_transmit_receive(*I2cDev, write_buffer, size, read_buffer, size, timeoutMs));
}

esp_err_t I2c_Receive(I2c_Dev* I2cDev, uint8_t* read_buffer, size_t size, int timeoutMs)
{
    return !(i2c_master_receive(*I2cDev, read_buffer, size, timeoutMs));
}