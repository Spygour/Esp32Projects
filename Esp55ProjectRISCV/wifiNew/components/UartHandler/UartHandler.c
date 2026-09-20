#include "UartHandler.h"
#include "esp_log.h"
#include "esp_log_level.h"

#define UART_NO_PIN (-1)

static const char *TAG = "UART_GENERIC";

typedef struct {
    uart_port_t uart_num;
    QueueHandle_t queue;
    uint8_t *rx_buffer;
    int buffer_size;
    UartHandler_Callback_t callback;
} UartHandler_Context_t;

static UartHandler_Context_t UartHandler_Context;

static uint16_t *UartHandler_PacketSize;


void UartHandler_MainTask(void *arg) 
{
    uart_event_t event;

    while (1) 
    {
        if (xQueueReceive(UartHandler_Context.queue, &event, portMAX_DELAY)) {
            switch (event.type) 
            {
                case UART_DATA: 
                {
                    if (event.size > 0)
                    {
                        int len = event.size;
                        int read_len = uart_read_bytes(UartHandler_Context.uart_num, UartHandler_Context.rx_buffer, len, 0);
                        UartHandler_Context.callback(UartHandler_Context.rx_buffer, read_len);
                    }
                    break;
                }

                case UART_FIFO_OVF:
                case UART_BUFFER_FULL:
                    ESP_LOGW(TAG, "Buffer overflow! Flushing...");
                    uart_flush_input(UartHandler_Context.uart_num);
                    xQueueReset(UartHandler_Context.queue);
                    break;

                default:
                    break;
            }
        }
    }
}

esp_err_t UartHandler_Init(uart_config_t *uart_config, uart_port_t uart_num, uint8_t* rx_buf, int tx_pin, int rx_pin, int buf_size, UartHandler_Callback_t cb , uint16_t *fifoMaxSize) 
{
    UartHandler_Context.callback = cb;
    UartHandler_Context.buffer_size = buf_size;
    UartHandler_Context.rx_buffer = rx_buf;
    UartHandler_PacketSize = fifoMaxSize;
    UartHandler_Context.uart_num = uart_num;
    if (!UartHandler_Context.rx_buffer) return ESP_ERR_NO_MEM;

    ESP_ERROR_CHECK(uart_driver_install(UartHandler_Context.uart_num, buf_size, 0, 20, &UartHandler_Context.queue, 0));
    ESP_ERROR_CHECK(uart_param_config(UartHandler_Context.uart_num, uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UartHandler_Context.uart_num, tx_pin, rx_pin, UART_NO_PIN, UART_NO_PIN));

    return ESP_OK;
}

uint8_t UartHandler_ReadData(uint8_t* rx_buf, uint32_t size, uint16_t millisecs)
{
    uint8_t ReadData_Eval;
    int len = uart_read_bytes(UartHandler_Context.uart_num, rx_buf, size, millisecs / portTICK_PERIOD_MS);
    if (len == size)
    {
        ReadData_Eval = 1U;
    }
    else if (len > 0)
    {
        ReadData_Eval = 2U;
    }
    else
    {
        ReadData_Eval = 0U;
    }
    return ReadData_Eval;
}

void UartHandler_WriteData(uint8_t* tx_buf, uint32_t size, uint16_t millisecs)
{
    uart_write_bytes(UartHandler_Context.uart_num, tx_buf, size);
    ESP_ERROR_CHECK(uart_wait_tx_done(UartHandler_Context.uart_num, millisecs / portTICK_PERIOD_MS));
}


void UartHandler_TaskInit(void)
{
    xTaskCreate(UartHandler_MainTask, "Uart_MainTask", 4096, NULL, 10, NULL);
}
