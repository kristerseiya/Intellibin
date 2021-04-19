
#ifndef __PROJECT_H__
#define __PROJECT_H__

#include <esp_camera.h>
#include <esp_log.h>

#define WIFI_SSID   "Apt Big 10"
#define WIFI_PSWD   "B01l3rUp!"
#define SERVER_ADDR "http://192.168.1.122:8889/predict"
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
// void example_wifi_init(void);
// esp_err_t example_espnow_init(void);

#endif
