#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/projdefs.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "portmacro.h"
#include "portIsr.h"
#include "esp_log.h"

#define MOVEMENT_PIN  0
#define TAG "PortIsr"
static volatile int64_t time_changed;
static TaskHandle_t portIsr_taskHandler;

static door_info_t portIsr_doorInfo;

QueueHandle_t portIsr_door_info_queue;

static void IRAM_ATTR Port_MovementIsr(void* arg) 
{
    // Keep ISR short: just set a flag or give a semaphore
    TaskHandle_t task_loc = (TaskHandle_t)arg;

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    time_changed = esp_timer_get_time();
    // Notify the task
    vTaskNotifyGiveFromISR(task_loc, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void Port_MovementTask(void *parameters)
{
    while (1)
    {
      ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
      /* Update the door current state */
      portIsr_doorInfo.current_state = gpio_get_level(MOVEMENT_PIN);
      portIsr_doorInfo.previous_state = !portIsr_doorInfo.current_state;
      portIsr_doorInfo.last_time_changed = time_changed;
      ESP_LOGI(TAG, "Door indication sent with value %d!", portIsr_doorInfo.current_state);
      /* Somehow we send the value to the bluetooth */
      xQueueOverwrite(portIsr_door_info_queue, &portIsr_doorInfo);
    }
}

void Port_InitIsr(void) 
{

    portIsr_door_info_queue = xQueueCreate(1, sizeof(door_info_t));
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
    time_changed = esp_timer_get_time();

    /* get the current state of the gpio */
    portIsr_doorInfo.current_state = gpio_get_level(MOVEMENT_PIN);
    /* current is equal to previous for now */
    portIsr_doorInfo.previous_state = portIsr_doorInfo.current_state;
    // Create a FreeRTOS task
    BaseType_t result = xTaskCreate(
        Port_MovementTask,         // Task function
        "MovementTask",  // Name of the task
        4096,            // Stack size in bytes
        NULL,            // Parameters to the task
        6,               // Task priority
        &portIsr_taskHandler             // Task handle (optional)
    );
    // Install ISR service
    gpio_install_isr_service(0);
    gpio_isr_handler_add(MOVEMENT_PIN, &Port_MovementIsr, (void*)portIsr_taskHandler);
    if (result != pdPASS)
    {
        printf("Task failed to be created!\n");
    }
}
