#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/projdefs.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "portmacro.h"
#include "Port_Interrupt.h"

#define MOVEMENT_PIN  47

SemaphoreHandle_t movement_sem;

static volatile int64_t movement_time = 0;
static volatile uint64_t duration = 0;

static bool motion_active = false;  // 0 = low, 1 = high

static void IRAM_ATTR Port_MovementIsr(void* arg) 
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    int64_t movement_time_us = esp_timer_get_time();
    duration = movement_time_us - movement_time;  // duration in us
    movement_time = movement_time_us;  // update last timestamp
    xSemaphoreGiveFromISR(movement_sem, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}


void Port_MovementTask(void *parameters)
{
    while (1)
    {
        if (xSemaphoreTake(movement_sem, pdMS_TO_TICKS(100)) == pdTRUE)
        {
                
            int level = gpio_get_level(MOVEMENT_PIN);
            if (level == 1) {
                // Rising edge: motion started
                motion_active = true;
            } else {
                // Falling edge: motion ended
                motion_active = false;
            }
            printf("Motion pulse duration: %llu us\n", duration);
            printf("Movement is detected: %d\n", motion_active);
        }
    }
}

void Port_InitIsr(uint32_t* movement_flag) 
{
    movement_sem = xSemaphoreCreateBinary();
    gpio_config_t io_conf = 
    {
        .intr_type = GPIO_INTR_POSEDGE,   // interrupt on rising edge
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = 1ULL << MOVEMENT_PIN,
        .pull_up_en = 0,                  // if needed
        .pull_down_en = 0
    };
    gpio_config(&io_conf);

    // Initialize the first timing
    movement_time = esp_timer_get_time();
    // Install ISR service
    gpio_install_isr_service(0);
    gpio_isr_handler_add(MOVEMENT_PIN, &Port_MovementIsr, (void*)movement_flag);
    // Create a FreeRTOS task
    BaseType_t result = xTaskCreate(
        Port_MovementTask,         // Task function
        "MovementTask",  // Name of the task
        4096,            // Stack size in bytes
        NULL,            // Parameters to the task
        5,               // Task priority
        NULL             // Task handle (optional)
    );
    if (result != pdPASS)
    {
        printf("Task failed to be created!\n");
    }
}
