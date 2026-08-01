/*
 * SPDX-FileCopyrightText: 2022-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
/* ULP-RISC-V example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.

   This code runs on ULP-RISC-V  coprocessor
*/
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_mac.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "../components/Wifi/WifiHandler.h"
#include "../components/Wifi/mbedtlsApp.h"


uint32_t mov_flag;
const char* Tag = "EnaTriaEnaduo";
int app_main (void)
{
     MX_MBEDTLS_Init();
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(Tag, "ESP_WIFI_MODE_AP");

    Wifi_Init();
    MX_MBEDTLS_INIT_HW();
    /* ulp_riscv_halt() is called automatically when main exits */
    return 0;
}
