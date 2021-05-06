/* UART Events Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"

#include "project.h"

static const char *TAG = "app_main";

VL53L0X_Dev_t tof_device1;
VL53L0X_Dev_t tof_device2;

void task(const char* message) {
    lcd_write_instruction(0b00000001);
    //lcd_clear();
    vTaskDelay(5 / portTICK_PERIOD_MS);
    lcd_write_instruction(0b10000000);
    // lcd_go_to_line1();
    vTaskDelay(5 / portTICK_PERIOD_MS);
    lcd_print((uint8_t*)message);
    mcpwm_servo_control(message[0]);
    char capacity_message[16];
    if (message[0] == 'R') {
      uint16_t result_mm1 = 0;
      bool res1 = vl53l0x_read(&tof_device2, &result_mm1);
      if(res1) {
        printf("Measured: %d[mm]", (int)result_mm1);
        int a = ((530 - (float)result_mm1) / 530) * 100;
        if (a < 0) {
          a = 0;
        }
        http_get_test1(a);
        printf("Recyclable sent: %d\n", a);
        sprintf(capacity_message, "%d%%", a);
      }
      else {
        printf("Couldn't read value recyclable\n");
        sprintf(capacity_message, "Measure failed");
      }
    }
    else {
      uint16_t result_mm2 = 0;
      bool res2 = vl53l0x_read(&tof_device1, &result_mm2);
      if(res2) {
        printf("Measured: %d[mm]", (int)result_mm2);
        int b = ((530 - (float)result_mm2) / 530) * 100;
        if (b < 0) {
          b = 0;
        }
        http_get_test2(b);
        printf("Non recyclable sent: %d\n", b);
        sprintf(capacity_message, "%d%%", b);
      }
      else {
        printf("Couldn't read value non recyclable\n");
        sprintf(capacity_message, "Measure failed");
      }
    }
    lcd_go_to_line2();
    lcd_print((uint8_t*)capacity_message);
    vTaskDelay(1000 / portTICK_RATE_MS);
}

void app_main(void)
{
    mcpwm_example_gpio_initialize();
    init_lcd();
    lcd_write_instruction(0b10000000);
    char boot_message[] = "Booting...";
    lcd_print((uint8_t*)boot_message);

    vTaskDelay(10000 / portTICK_RATE_MS);
    lcd_write_instruction(0b00000001);

    // init_lcd();
    connect2wifi();

    if (!init_vl53l0x(&tof_device2, I2C_PORT2, PIN_SDA2, PIN_SCL2)) {
      ESP_LOGE(TAG, "Failed to initialize VL53L0X 2 :(");
      //vTaskDelay(portMAX_DELAY);
    } else {
      ESP_LOGI(TAG, "VL53L0X 2 initialized");
    }

    if (!init_vl53l0x(&tof_device1, I2C_PORT1, PIN_SDA1, PIN_SCL1)) {
      ESP_LOGE(TAG, "Failed to initialize VL53L0X 1 :(");
      //vTaskDelay(portMAX_DELAY);
    } else {
      ESP_LOGI(TAG, "VL53L0X 1 initialized");
    }

    init_uart();

    create_task(task);
}
