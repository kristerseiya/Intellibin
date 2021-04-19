/* ESP HTTP Client Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_tls.h"

#include "esp_http_client.h"
#include "esp_camera.h"

#include "project.h"

#define MAX_HTTP_RECV_BUFFER 512
#define MAX_HTTP_OUTPUT_BUFFER 512
static const char *TAG = "HTTP_CLIENT";

/*
 *  http_native_request() demonstrates use of low level APIs to connect to a server,
 *  make a http request and read response. Event handler is not used in this case.
 *  Note: This approach should only be used in case use of low level APIs is required.
 *  The easiest way is to use esp_http_perform()
 */
size_t http_request_post(camera_fb_t* image_data, char* output_buffer)
{
    // message = NULL;
    // char output_buffer[MAX_HTTP_OUTPUT_BUFFER] = {0};   // Buffer to store response of http request
    // output_buffer = (char*)malloc(MAX_HTTP_OUTPUT_BUFFER*sizeof(char));
    int content_length = 0;
    esp_http_client_config_t config = {
        // .url = "http://192.168.1.122:8889/predict",
        // .url = "http://172.20.10.2:8889/predict",
        .url = SERVER_ADDR,
        .user_data = output_buffer,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);

    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, (const char*)image_data->buf, image_data->len);
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP POST Status = %d", esp_http_client_get_status_code(client));
        content_length = esp_http_client_get_content_length(client);
        content_length = content_length > MAX_HTTP_OUTPUT_BUFFER ? MAX_HTTP_OUTPUT_BUFFER : content_length;
        esp_http_client_read(client, output_buffer, content_length);
        output_buffer[content_length] = '\0';
        ESP_LOGI(TAG, "Message: %s", output_buffer);
        // message = output_buffer;
    } else {
        ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
        // free(output_buffer);
        // message = NULL;
    }
    // free(output_buffer);
    // free(post_data);
    esp_http_client_cleanup(client);
    return content_length;
}

void init_http(void) {
    ESP_ERROR_CHECK(esp_netif_init());
}
