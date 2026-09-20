#include "nvs_flash.h"
#include "../components/Wifi/WifiHandler.h"
#include "../components/MovementSensor/Port_Interrupt.h"
#include <stdint.h>


uint32_t g_movement_flag;

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    Port_InitIsr(&g_movement_flag);
    Wifi_Init();
}
