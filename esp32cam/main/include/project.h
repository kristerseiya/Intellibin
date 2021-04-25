
#ifndef __PROJECT_H__
#define __PROJECT_H__

#include "esp_camera.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "vl53l0x_platform.h"

// #define WIFI_SSID   "Apt Big 10"
// #define WIFI_PSWD   "B01l3rUp!"
#define WIFI_SSID   "The Twelve"
#define WIFI_PSWD   "upxbrmcamkz4d"
// #define SERVER_ADDR "http://192.168.1.122:8889/predict"
#define SERVER_ADDR "http://172.20.10.2:8889/predict"
#define PIN_SCL     GPIO_NUM_14
#define PIN_SDA     GPIO_NUM_15
#define PIN_UART_TX GPIO_NUM_12
#define PIN_UART_RX GPIO_NUM_13
#define JPEG_RES    FRAMESIZE_VGA
// #define JPEG_RES    FRAMESIZE_240X240
#define JPEG_QUAL   10

void connect2wifi(void);
void init_http(void);
size_t http_request_post(camera_fb_t*, char*);
esp_err_t init_camera(void);
void init_led(void);
void init_uart(void);
void uart_send(const char*, size_t);
bool init_vl53l0x(VL53L0X_Dev_t* vl53l0x_dev,
                  i2c_port_t i2c_port,
                  gpio_num_t pin_sda,
                  gpio_num_t pin_scl);
bool vl53l0x_read(VL53L0X_Dev_t* vl53lox_dev, uint16_t *pRangeMilliMeter);

// void example_wifi_init(void);
// esp_err_t example_espnow_init(void);

#endif
