

#include "driver/gpio.h"

#define PIN_LCD_D7  GPIO_NUM_14
#define PIN_LCD_D6  GPIO_NUM_32
#define PIN_LCD_D5  GPIO_NUM_15
#define PIN_LCD_D4  GPIO_NUM_33
#define PIN_LCD_E   GPIO_NUM_27
#define PIN_LCD_RW  GPIO_NUM_12
#define PIN_LCD_RS  GPIO_NUM_13

void init_lcd(void);
void lcd_print(uint8_t*);
void lcd_write_instruction(uint8_t);
void lcd_clear(void);
void lcd_clear_line1(void);
void lcd_clear_line2(void);
void lcd_go_to_line1(void);
void lcd_go_to_line2(void);
