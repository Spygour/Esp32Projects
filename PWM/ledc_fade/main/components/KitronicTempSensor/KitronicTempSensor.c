#include "KitronicTempSensor.h"

static I2c_Bus_Master KitronicTempSensorI2cBus;
static I2c_Dev        KitronicTempSensorI2cDev;
static float          KitronicTempSensorTemp;

void KitronicTempSensor_Init(void)
{
    I2c_Init(&KitronicTempSensorI2cBus, &KitronicTempSensorI2cDev, 0x48, 1000000);
}

void KitronicTempSensor_ReadTemp(void)
{
    static uint8_t ReceiveBytes[2];
    I2c_Receive(&KitronicTempSensorI2cDev, ReceiveBytes, 2, 100);
    uint16_t TempSum;
    TempSum = ((ReceiveBytes[0]<<8)|ReceiveBytes[1])>>4;
    KitronicTempSensorTemp = (float)TempSum* 0.0625;
}

float KitronicTempSensor_GetTemp(void)
{
    return KitronicTempSensorTemp;
}

