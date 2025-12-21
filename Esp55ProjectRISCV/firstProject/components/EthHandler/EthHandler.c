/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
/* eth2ap (Ethernet to Wi-Fi AP packet forwarding) Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include <stdlib.h>
#include "sdkconfig.h"
#include "esp_log.h"
#include "esp_eth_driver.h"
#include "esp_event.h"
#include "ethernet_init.h"
#include "EthHandler.h"

#define ETHERTYPE_SIZE 12

static const char *TAG = "eth2ap_example";
static esp_eth_handle_t s_eth_handle = NULL;
static QueueHandle_t flow_control_queue = NULL;
static bool s_sta_is_connected = false;
static bool s_ethernet_is_connected = false;
static uint8_t s_eth_mac[6];

static uint8_t EthernetBuff[512];

typedef struct ethernet_packet_s
{
    uint8_t srcAddr[6];
    uint8_t dstAddr[6];
    uint16_t ethertype[2];
    uint8_t Payload[128];
    size_t payloadSize;

}ethernet_packet;


static ethernet_packet Eth_Packet = 
{
    {0x30, 0xED, 0xA0, 0xE2, 0x16, 0x67},
    {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
    {0x08, 0x00},
    {0xFF},
    60
};

#define FLOW_CONTROL_QUEUE_TIMEOUT_MS (100)
#define FLOW_CONTROL_QUEUE_LENGTH (40)
#define FLOW_CONTROL_WIFI_SEND_TIMEOUT_MS (100)

typedef struct {
    void *packet;
    uint16_t length;
} flow_control_msg_t;


static esp_err_t EthHandler_TransmitPacket(esp_eth_handle_t hdl, uint8_t *buffer, uint32_t length, void *priv)
{
    uint8_t *data = (uint8_t *)buffer;

    // Log first few bytes
    ESP_LOGI("RX", "Received packet (%lu bytes): %02X %02X %02X %02X %02X %02X ...",
             length, data[10], data[11], data[12], data[13], data[14], data[15]);

    // Example: check if it's an IP packet (Ethertype 0x0800)
    if (length >= 14 && data[12] == 0x08 && data[13] == 0x00) {
        ESP_LOGI("RX", "IP packet received");
    }

    // Always free the buffer after processing!
    free(buffer);

    ESP_ERROR_CHECK(esp_eth_transmit(s_eth_handle, &EthernetBuff[0], Eth_Packet.payloadSize + ETHERTYPE_SIZE));

    return ESP_OK;
}

// Event handler for Ethernet
static void EthHandler_EventHandler(void *arg, esp_event_base_t event_base,
                              int32_t event_id, void *event_data)
{
    switch (event_id) {
    case ETHERNET_EVENT_CONNECTED:
        ESP_LOGI(TAG, "Ethernet Link Up");
        s_ethernet_is_connected = true;
        esp_eth_ioctl(s_eth_handle, ETH_CMD_G_MAC_ADDR, s_eth_mac);
        ESP_ERROR_CHECK(esp_eth_transmit(s_eth_handle, &EthernetBuff[0], Eth_Packet.payloadSize + ETHERTYPE_SIZE));
        break;
    case ETHERNET_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "Ethernet Link Down");
        s_ethernet_is_connected = false;
        break;
    case ETHERNET_EVENT_START:
        ESP_LOGI(TAG, "Ethernet Started");
        break;
    case ETHERNET_EVENT_STOP:
        ESP_LOGI(TAG, "Ethernet Stopped");
        break;
    default:
        break;
    }
}

void EthHandler_Init(void)
{
    uint8_t eth_port_cnt = 0;
    esp_eth_handle_t *eth_handles;
    ESP_ERROR_CHECK(example_eth_init(&eth_handles, &eth_port_cnt));
    if (eth_port_cnt > 1) {
        ESP_LOGW(TAG, "multiple Ethernet devices detected, the first initialized is to be used!");
    }
    s_eth_handle = eth_handles[0];
    free(eth_handles);
    /* Print mac address for the friend ecu */
    uint8_t mac_return[6];
    ESP_ERROR_CHECK(esp_eth_ioctl(s_eth_handle, ETH_CMD_G_MAC_ADDR, mac_return));
    ESP_LOGI(TAG, "Ethernet MAC before start: %02X:%02X:%02X:%02X:%02X:%02X",
    mac_return[0], mac_return[1], mac_return[2], mac_return[3], mac_return[4], mac_return[5]);
    //uint8_t mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    //ESP_ERROR_CHECK(esp_eth_ioctl(s_eth_handle, ETH_CMD_S_MAC_ADDR, mac));
    ESP_ERROR_CHECK(esp_eth_ioctl(s_eth_handle, ETH_CMD_G_MAC_ADDR, mac_return));
    ESP_LOGI(TAG, "Ethernet MAC before start: %02X:%02X:%02X:%02X:%02X:%02X",
    mac_return[0], mac_return[1], mac_return[2], mac_return[3], mac_return[4], mac_return[5]);
    /* Here We have to set the receive irs */
    ESP_ERROR_CHECK(esp_eth_update_input_path(s_eth_handle, EthHandler_TransmitPacket, NULL));

    /* Prepare the ethernet to await for the friend ecu to wake up */
    bool eth_autonego = true;
    ESP_ERROR_CHECK(esp_eth_ioctl(s_eth_handle, ETH_CMD_S_AUTONEGO, &eth_autonego));
    bool eth_promiscuous = true;
    ESP_ERROR_CHECK(esp_eth_ioctl(s_eth_handle, ETH_CMD_S_PROMISCUOUS, &eth_promiscuous));
    /* Create interrupt state machine like, any time an event happens it jumps inside */
    ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, EthHandler_EventHandler, NULL));
    ESP_ERROR_CHECK(esp_eth_start(s_eth_handle));
}
