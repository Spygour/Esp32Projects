#include "driver/spi_master.h"
#include "driver/gpio.h"
#include <string.h>
#include "Spi.h"

#define PIN_NUM_MOSI 23
#define PIN_NUM_CLK  18
#define PIN_NUM_CS   5

void ExampleMain(void)
{
    spi_bus_config_t buscfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = -1,   // if not used
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4096,
    };

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 1 * 1000 * 1000,
        .mode = 0,
        .spics_io_num = PIN_NUM_CS,
        .queue_size = 1,
    };

    spi_device_handle_t spi;

    spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);
    spi_bus_add_device(SPI2_HOST, &devcfg, &spi);

    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length = 8 * 4;
    uint8_t data[4] = {0xAA, 0xBB, 0xCC, 0xDD};
    t.tx_buffer = data;

    esp_err_t ret = spi_device_transmit(spi, &t);
    if (ret == ESP_OK) {
        printf("SPI transmit successful.\n");
    } else {
        printf("SPI transmit failed.\n");
    }
}