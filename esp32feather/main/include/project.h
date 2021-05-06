

#include "driver/gpio.h"
#include "driver/i2c.h"
#include "vl53l0x_platform.h"

#define PIN_LCD_D7  GPIO_NUM_14
#define PIN_LCD_D6  GPIO_NUM_32
#define PIN_LCD_D5  GPIO_NUM_15
#define PIN_LCD_D4  GPIO_NUM_33
#define PIN_LCD_E   GPIO_NUM_27
#define PIN_LCD_RW  GPIO_NUM_12
#define PIN_LCD_RS  GPIO_NUM_13
// #define PIN_LCD_D7  GPIO_NUM_13
// #define PIN_LCD_D6  GPIO_NUM_12
// #define PIN_LCD_D5  GPIO_NUM_27
// #define PIN_LCD_D4  GPIO_NUM_33
// #define PIN_LCD_E   GPIO_NUM_15
// #define PIN_LCD_RW  GPIO_NUM_32
// #define PIN_LCD_RS  GPIO_NUM_14

#define I2C_PORT1 I2C_NUM_0
#define PIN_SDA1 GPIO_NUM_23
#define PIN_SCL1 GPIO_NUM_22
#define I2C_PORT2 I2C_NUM_1
#define PIN_SDA2 GPIO_NUM_4
#define PIN_SCL2 GPIO_NUM_25

#define PIN_MOTOR1 GPIO_NUM_21
#define PIN_MOTOR2 GPIO_NUM_26

// #define WIFI_SSID "OnePlus 7 Pro"
// #define WIFI_PSWD "soccer3006"
#define WIFI_SSID "Aathavan"
#define WIFI_PSWD "Purdue123"

#define THINKSPEAK_SERVER "https://api.thingspeak.com/update"
#define THINKSPEAK_API_KEY "FUMY2NOXR6FCKVWO"

void init_lcd(void);
void lcd_print(uint8_t*);
void lcd_write_instruction(uint8_t);
void lcd_clear(void);
void lcd_clear_line1(void);
void lcd_clear_line2(void);
void lcd_go_to_line1(void);
void lcd_go_to_line2(void);

void mcpwm_example_gpio_initialize();
uint32_t servo_per_degree_init(uint32_t);
void mcpwm_servo_control(char);

void connect2wifi(void);
void http_get_test1(int);
void http_get_test2(int);

void init_uart(void);
void create_task(void(*task)(const char*));

bool init_vl53l0x(VL53L0X_Dev_t*, i2c_port_t, gpio_num_t, gpio_num_t);
bool vl53l0x_read(VL53L0X_Dev_t*, uint16_t*);
