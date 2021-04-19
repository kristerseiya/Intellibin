/**
 * This example takes a picture every 5s and print its size on serial monitor.
 */

// =============================== SETUP ======================================

// 1. Board setup (Uncomment):
// #define BOARD_WROVER_KIT
// #define ESP_EYE

#include "project.h"

// #define BOARD_ESP32CAM_AITHINKER

/**
 * 2. Kconfig setup
 *
 * If you have a Kconfig file, copy the content from
 *  https://github.com/espressif/esp32-camera/blob/master/Kconfig into it.
 * In case you haven't, copy and paste this Kconfig file inside the src directory.
 * This Kconfig file has definitions that allows more control over the camera and
 * how it will be initialized.
 */

/**
 * 3. Enable PSRAM on sdkconfig:
 *
 * CONFIG_ESP32_SPIRAM_SUPPORT=y
 *
 * More info on
 * https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/kconfig.html#config-esp32-spiram-support
 */

// ================================ CODE ======================================

#include <esp_event_loop.h>
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include <string.h>

#include <esp_wifi.h>
#include <esp_event_loop.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_camera.h"
#include "driver/ledc.h"
#include "driver/gpio.h"

// // WROVER-KIT PIN Map
// #ifdef BOARD_WROVER_KIT
//
// #define CAM_PIN_PWDN -1  //power down is not used
// #define CAM_PIN_RESET -1 //software reset will be performed
// #define CAM_PIN_XCLK 21
// #define CAM_PIN_SIOD 26
// #define CAM_PIN_SIOC 27
//
// #define CAM_PIN_D7 35
// #define CAM_PIN_D6 34
// #define CAM_PIN_D5 39
// #define CAM_PIN_D4 36
// #define CAM_PIN_D3 19
// #define CAM_PIN_D2 18
// #define CAM_PIN_D1 5
// #define CAM_PIN_D0 4
// #define CAM_PIN_VSYNC 25
// #define CAM_PIN_HREF 23
// #define CAM_PIN_PCLK 22
//
// #endif

// ESP32Cam (AiThinker) PIN Map
// #ifdef BOARD_ESP32CAM_AITHINKER

#define CAM_PIN_PWDN 32
#define CAM_PIN_RESET -1 //software reset will be performed
#define CAM_PIN_XCLK 0
#define CAM_PIN_SIOD 26
#define CAM_PIN_SIOC 27

#define CAM_PIN_D7 35
#define CAM_PIN_D6 34
#define CAM_PIN_D5 39
#define CAM_PIN_D4 36
#define CAM_PIN_D3 21
#define CAM_PIN_D2 19
#define CAM_PIN_D1 18
#define CAM_PIN_D0 5
#define CAM_PIN_VSYNC 25
#define CAM_PIN_HREF 23
#define CAM_PIN_PCLK 22

// #endif

// #ifdef ESP_EYE
//
// #define CAM_PIN_PWDN    -1
// #define CAM_PIN_RESET   -1
// #define CAM_PIN_XCLK    4
// #define CAM_PIN_SIOD    18
// #define CAM_PIN_SIOC    23
//
// #define CAM_PIN_D7      36
// #define CAM_PIN_D6      37
// #define CAM_PIN_D5      38
// #define CAM_PIN_D4      39
// #define CAM_PIN_D3      35
// #define CAM_PIN_D2      14
// #define CAM_PIN_D1      13
// #define CAM_PIN_D0      34
// #define CAM_PIN_VSYNC   5
// #define CAM_PIN_HREF    27
// #define CAM_PIN_PCLK    25
//
// #endif

#define CONFIG_LED_LEDC_TIMER   1
#define CONFIG_LED_LEDC_CHANNEL   1
#define CONFIG_LED_LEDC_PIN   4
#define CONFIG_LED_DUTY 10
#define CONFIG_LED_MAX_INTENSITY  255

static const char *TAG = "example:take_picture";

static camera_config_t camera_config = {

    .pin_pwdn = CAM_PIN_PWDN,
    .pin_reset = CAM_PIN_RESET,
    .pin_xclk = CAM_PIN_XCLK,
    .pin_sscb_sda = CAM_PIN_SIOD,
    .pin_sscb_scl = CAM_PIN_SIOC,

    .pin_d7 = CAM_PIN_D7,
    .pin_d6 = CAM_PIN_D6,
    .pin_d5 = CAM_PIN_D5,
    .pin_d4 = CAM_PIN_D4,
    .pin_d3 = CAM_PIN_D3,
    .pin_d2 = CAM_PIN_D2,
    .pin_d1 = CAM_PIN_D1,
    .pin_d0 = CAM_PIN_D0,
    .pin_vsync = CAM_PIN_VSYNC,
    .pin_href = CAM_PIN_HREF,
    .pin_pclk = CAM_PIN_PCLK,

    //XCLK 20MHz or 10MHz for OV2640 double FPS (Experimental)
    .xclk_freq_hz = 20000000,
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,

    .pixel_format = PIXFORMAT_JPEG, //YUV422,GRAYSCALE,RGB565,JPEG
    // .pixel_format = PIXFORMAT_RGB565,
    .frame_size = JPEG_RES,    //QQVGA-UXGA Do not use sizes above QVGA when not JPEG
    // .frame_size = FRAMESIZE_240X240, // FRAMESIZE_240X240,

    .jpeg_quality = JPEG_QUAL, //12, //0-63 lower number means higher quality
    .fb_count = 1       //if more than one, i2s runs in continuous mode. Use only with JPEG
};

esp_err_t init_camera()
{
    //initialize the camera
    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Camera Init Failed");
        return err;
    }

    sensor_t* s = esp_camera_sensor_get();
    s->set_brightness(s, 0);

    return ESP_OK;
}

void init_led() {
  // gpio_config_t conf;
  // conf.mode = GPIO_MODE_INPUT;
  // conf.pull_up_en = GPIO_PULLUP_ENABLE;
  // conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  // conf.intr_type = GPIO_INTR_DISABLE;
  // conf.pin_bit_mask = 1LL << 13;
  // gpio_config(&conf);
  // conf.pin_bit_mask = 1LL << 14;
  // gpio_config(&conf);
  gpio_pad_select_gpio(CONFIG_LED_LEDC_PIN);
  gpio_set_direction(CONFIG_LED_LEDC_PIN, GPIO_MODE_OUTPUT);
  ledc_timer_config_t ledc_timer = {
    .duty_resolution = LEDC_TIMER_8_BIT,            // resolution of PWM duty
    .freq_hz         = 100,                        // frequency of PWM signal
    .speed_mode      = LEDC_LOW_SPEED_MODE,  // timer mode
    .timer_num       = LEDC_TIMER_1        // timer index
  };
  ledc_channel_config_t ledc_channel = {
    .channel    = LEDC_CHANNEL_1,
    .duty       = 1,
    .gpio_num   = CONFIG_LED_LEDC_PIN,
    .speed_mode = LEDC_LOW_SPEED_MODE,
    .hpoint     = 0,
    .timer_sel  = CONFIG_LED_LEDC_TIMER
  };
  // #ifdef CONFIG_LED_LEDC_HIGH_SPEED_MODE
  ledc_timer.speed_mode = ledc_channel.speed_mode = LEDC_HIGH_SPEED_MODE;
  // #endif
  switch (ledc_timer_config(&ledc_timer)) {
    case ESP_ERR_INVALID_ARG: ESP_LOGE(TAG, "ledc_timer_config() parameter error"); break;
    case ESP_FAIL: ESP_LOGE(TAG, "ledc_timer_config() Can not find a proper pre-divider number base on the given frequency and the current duty_resolution"); break;
    case ESP_OK: if (ledc_channel_config(&ledc_channel) == ESP_ERR_INVALID_ARG) {
        ESP_LOGE(TAG, "ledc_channel_config() parameter error");
      }
      break;
    default: break;
  }
}

// void enable_led(bool en)
// { // Turn LED On or Off
//     int duty = en ? CONFIG_LED_DUTY : 0;
//     if (en && (CONFIG_LED_DUTY > CONFIG_LED_MAX_INTENSITY))
//     {
//         duty = CONFIG_LED_MAX_INTENSITY;
//     }
//     ledc_set_duty(LEDC_LOW_SPEED_MODE, CONFIG_LED_LEDC_CHANNEL, duty);
//     ledc_update_duty(LEDC_LOW_SPEED_MODE, CONFIG_LED_LEDC_CHANNEL);
//     ESP_LOGI(TAG, "Set LED intensity to %d", duty);
// }

//
// extern "C" void app_main()
// {
//     connect2wifi();
//     init_http();
//     init_camera();
//     init_led();
//
//     //
//     // while (1)
//     // {
//     //     ESP_LOGI(TAG, "Taking picture...");
//     //     camera_fb_t *pic = esp_camera_fb_get();
//     //
//     //     // use pic->buf to access the image
//     //     ESP_LOGI(TAG, "Picture taken! Its size was: %zu bytes", pic->len);
//     //     http_request_post(pic);
//     //     vTaskDelay(1000 / portTICK_RATE_MS);
//     // }
//
// }
