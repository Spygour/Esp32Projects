#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/sockets.h"
#include "lwip/inet.h"

#define EXAMPLE_WIFI_SSID      "MyWIFI"
#define EXAMPLE_WIFI_PASS      "MYPASS"

static const char *TAG = "wifi_sta";

static char wifi_rx[40];

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED) {
        ESP_LOGI(TAG, "Connected to router");
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        wifi_event_sta_disconnected_t* event = (wifi_event_sta_disconnected_t*) event_data;
        ESP_LOGI(TAG, "Disconnected. Reason: %d", event->reason);
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));

        // Send a UDP message to router (gateway)
        int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        struct sockaddr_in dest_addr = {
            .sin_family = AF_INET,
            .sin_port = htons(1234),
        };
        inet_pton(AF_INET, "192.168.1.2",&dest_addr.sin_addr); //event->ip_info.gw.addr; // router IP
        int err = connect(sock, (struct sockaddr*)&dest_addr, sizeof(dest_addr));
        if (err != 0) {
            ESP_LOGE(TAG, "Socket unable to connect: errno %d", errno);
        }
        char msg[] = "Gamiesai Aleura!\n";
        send(sock, msg, strlen(msg), 0);
        char msg2[] = "Mitsotaki gamiesai!";
        send(sock, msg2, strlen(msg2), 0);
        if (err < 0) {
            ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
        } else {
            ESP_LOGI(TAG, "Message sent to %s", ip4addr_ntoa((const ip4_addr_t*)&event->ip_info.ip));
        }
        int len = recv(sock, wifi_rx, sizeof(wifi_rx) - 1, 0);
        if (len > 0) {
            wifi_rx[len] = '\0';  // null-terminate so it's safe to print as string
            ESP_LOGI(TAG, "Received: %s", wifi_rx);
        } else if (len == 0) {
            ESP_LOGW(TAG, "Received empty packet");
        } else {
            ESP_LOGW(TAG, "No response or timeout, errno=%d", errno);
        }
        close(sock);
    }   
}

void wifi_init_sta(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_WIFI_SSID,
            .password = EXAMPLE_WIFI_PASS,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Wi-Fi STA init finished. Connecting to SSID:%s", EXAMPLE_WIFI_SSID);
}

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    wifi_init_sta();
}
