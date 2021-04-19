
#include "VL53L0X.h"
#include "esp_log.h"
extern "C" {
  #include "project.h"
  #include "vl53l0x_api.h"
  #include "driver/i2c.h"
}

/* config */
#define I2C_PORT  I2C_NUM_0
// #define PIN_SDA GPIO_NUM_13
// #define PIN_SCL GPIO_NUM_12

static const char *TAG = "app_main";

extern "C" void app_main()
{
    //
    //vTaskDelay(3500 / portTICK_RATE_MS);
    connect2wifi();
    init_http();
    init_camera();
    init_uart();

    // while (1) {
    //   ESP_LOGI(TAG, "Taking picture...");
    //   camera_fb_t *pic = esp_camera_fb_get();
    //
    //       // // use pic->buf to access the image
    //   ESP_LOGI(TAG, "Picture taken! Its size was: %zu bytes", pic->len);
    //   http_request_post(pic);
    //   vTaskDelay(3500 / portTICK_RATE_MS);
    // }

    VL53L0X vl(I2C_NUM_0);

    vl.i2cMasterInit(PIN_SDA, PIN_SCL);
    if (!vl.init()) {
      ESP_LOGE(TAG, "Failed to initialize VL53L0X 1 :(");
      vTaskDelay(portMAX_DELAY);
    }

    char* response = (char*)malloc(512);


    while (1) {
      /* measurement */
      uint16_t result_mm = 0;
      // TickType_t tick_start = xTaskGetTickCount();
      bool res = vl.read(&result_mm);
      // TickType_t tick_end = xTaskGetTickCount();
      // int took_ms = ((int)tick_end - tick_start) / portTICK_PERIOD_MS;
      if (res) {
        ESP_LOGI(TAG, "Range: %d [mm]", (int)result_mm);
        if (result_mm < 100) {
          ESP_LOGI(TAG, "Taking picture...");
          camera_fb_t *pic = esp_camera_fb_get();

          // // use pic->buf to access the image
          ESP_LOGI(TAG, "Picture taken! Its size was: %zu bytes", pic->len);
          // char* response = NULL;
          size_t content_length = http_request_post(pic, response);
          if (content_length > 0) {
            uart_send(response, content_length);
            // free(response);
          }
          vTaskDelay(5000 / portTICK_RATE_MS);
          }
      }
      vTaskDelay(600 / portTICK_RATE_MS);
    }

}
