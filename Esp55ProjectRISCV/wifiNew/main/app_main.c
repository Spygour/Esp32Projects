#include "nvs_flash.h"
#include "WifiHandler.h"
#include "Port_Interrupt.h"
#include "DfRobotLidar7.h"
#include <stdint.h>


uint32_t g_movement_flag;

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    //DfRobotLidar7_Init(eLidar07Continuous);

    Port_InitIsr(&g_movement_flag);
    Wifi_Init();
}
