
#include "esp_log.h"
#include "project.h"
#include "driver/i2c.h"


static const char *TAG = "app_main";

void app_main()
{

    connect2wifi();
    init_http();
    init_camera();
    init_uart();

    VL53L0X_Dev_t tof_device;
    if (!init_vl53l0x(&tof_device, I2C_PORT, PIN_SDA, PIN_SCL)) {
      ESP_LOGE(TAG, "Failed to initialize VL53L0X 1 :(");
      vTaskDelay(portMAX_DELAY);
    }

    char* response = (char*)malloc(MAX_HTTP_OUTPUT_BUFFER);

    while (1) {
      /* measurement */
      uint16_t result_mm = 0;
      bool res = vl53l0x_read(&tof_device, &result_mm);
      if (res) {
        ESP_LOGI(TAG, "Range: %d [mm]", (int)result_mm);
        if (result_mm < 100) {
          ESP_LOGI(TAG, "Taking picture...");
          camera_fb_t *pic = esp_camera_fb_get();

          // // use pic->buf to access the image
          ESP_LOGI(TAG, "Picture taken! Its size was: %zu bytes", pic->len);
          size_t content_length = http_request_post(pic, response);
          if (content_length > 0) {
            uart_send(response, content_length);
          }
          vTaskDelay(10000 / portTICK_RATE_MS);
          }
      }
      vTaskDelay(600 / portTICK_RATE_MS);
    }

    free(response);
}
