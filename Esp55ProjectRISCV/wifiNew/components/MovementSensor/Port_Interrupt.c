#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/projdefs.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "portmacro.h"
#include "Port_Interrupt.h"

#define MOVEMENT_PIN  5  // example GPIO

SemaphoreHandle_t movement_sem;
volatile int64_t movement_time_us = 0;  // timestamp in microseconds

static int64_t movement_time = 0;
static uint64_t duration = 0;

static int motion_active = 0;  // 0 = low, 1 = high

static void IRAM_ATTR Port_MovementIsr(void* arg) 
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    int level = gpio_get_level(MOVEMENT_PIN);

    movement_time_us = esp_timer_get_time();

    if (level == 1) {
        // Rising edge: motion started
        motion_active = 1;
    } else {
        // Falling edge: motion ended
        motion_active = 0;
        duration = movement_time_us - movement_time;  // duration in us
        xSemaphoreGiveFromISR(movement_sem, &xHigherPriorityTaskWoken);
    }

    movement_time = movement_time_us;  // update last timestamp
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}


void Port_MovementTask(void *parameters)
{
    while (1)
    {
        if (xSemaphoreTake(movement_sem, pdMS_TO_TICKS(50)) == pdTRUE)
        {
            printf("Motion pulse duration: %llu us\n", duration);
            printf("Movement is detected\n");
        }
    }
}

void Port_InitIsr(uint32_t* movement_flag) 
{
    movement_sem = xSemaphoreCreateBinary();
    gpio_config_t io_conf = 
    {
        .intr_type = GPIO_INTR_ANYEDGE,   // interrupt on rising edge
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = 1ULL << MOVEMENT_PIN,
        .pull_up_en = 1,                  // if needed
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
