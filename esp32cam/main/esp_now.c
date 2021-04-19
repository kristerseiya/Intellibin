#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_now.h"
#include "esp_crc.h"
#include "espnow_example.h"

static const char *TAG = "espnow_example";
typedef struct struct_message {
  char a[32];
  int b;
  float c;
  bool e;
} struct_message;
struct_message myData;
static uint8_t mac_addr[] = { 0xAC, 0x67, 0xB2, 0x09, 0xDD, 0xE4 };

void example_wifi_init(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    ESP_ERROR_CHECK( esp_wifi_set_mode(ESPNOW_WIFI_MODE) );
    ESP_ERROR_CHECK( esp_wifi_start());

#if CONFIG_ESPNOW_ENABLE_LONG_RANGE
    ESP_ERROR_CHECK( esp_wifi_set_protocol(ESPNOW_WIFI_IF, WIFI_PROTOCOL_11B|WIFI_PROTOCOL_11G|WIFI_PROTOCOL_11N|WIFI_PROTOCOL_LR) );
#endif
}

static void example_espnow_send_cb(const uint8_t *mac_addr, esp_now_send_status_t status) {
    if (status == ESP_NOW_SEND_SUCCESS) {
        ESP_LOGI(TAG, "Delivery success");
    }
    else {
        ESP_LOGE(TAG, "Delivery failed");
    }
}

esp_err_t example_espnow_init(void) {
    ESP_ERROR_CHECK( esp_now_init() );
    ESP_ERROR_CHECK( esp_now_register_send_cb(example_espnow_send_cb) );
    esp_now_peer_info_t peerInfo;

    memcpy(peerInfo.peer_addr, mac_addr, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    // Add peer
    if (esp_now_add_peer(&peerInfo) != ESP_OK){
        ESP_LOGE(TAG, "Failed to add peer");
    }

    while (1) {
        strcpy(myData.a, "THIS IS A CHAR");
        myData.b = 190;
        myData.c = 1.2;
        myData.e = false;
        esp_err_t result = esp_now_send(mac_addr, (uint8_t *) &myData, sizeof(myData));
        if (result == ESP_OK) {
            ESP_LOGI(TAG, "Sent successfully");
        }
        else {
            ESP_LOGE(TAG, "Fail");
            // ESP_LOGE(result);
        }
        sleep(10);
    }
}

// void app_main() {
//  esp_err_t ret = nvs_flash_init();
//     if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
//         ESP_ERROR_CHECK( nvs_flash_erase() );
//         ret = nvs_flash_init();
//     }
//     ESP_ERROR_CHECK( ret );
//
//     example_wifi_init();
//     example_espnow_init();
// }
