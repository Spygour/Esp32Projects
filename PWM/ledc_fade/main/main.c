/* LEDC (LED Controller) fade example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
    
#include "components/Spi/Spi.h"
#include "components/Gptimer/Gptimer.h"
#include <stdint.h>
#include <stdio.h>
#include "components/esp32Types.h"


void app_main(void)
{
    GpTimerInit();
    waitMs(500);
    while(true)
    {
        printf("Sisixaxa\n");
        GpWaitMs(1000);
        printf("Auto etrekse\n");
        waitMs(100);
    }
}
