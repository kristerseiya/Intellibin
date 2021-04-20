/****************************************************************************
    LCD-AVR-4f.c  - Use an HD44780U based LCD with an Atmel ATmega processor

    Copyright (C) 2013 Donald Weiman    (weimandn@alfredstate.edu)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
/****************************************************************************
         File:    LCD-AVR-4f.c
         Date:    September 16, 2013

       Target:    ATmega328
     Compiler:    avr-gcc (AVR Studio 6)
       Author:    Donald Weiman

      Summary:    8-bit data interface, with busy flag implemented.
                  Any LCD pin can be connected to any available I/O port.
                  Includes a simple write string routine.
 */
/******************************* Program Notes ******************************

            This program uses a 4-bit data interface and it uses the busy
              flag to determine when the LCD controller is ready.  The LCD
              RW line (pin 5) must therefore be connected to the uP.

            The use of the busy flag does not mean that all of the software
              time delays have been eliminated.  There are still several
              required in the LCD initialization routine where the busy flag
              cannot yet be used.  These delays are have been implemented at
              least twice as long as called for in the data sheet to
              accommodate some of the out of spec displays that may show up.
              There are also some other software time delays required to
              implement timing requirements such as setup and hold times for
              the various control signals.

  ***************************************************************************

            The four data lines as well as the three control lines may be
              implemented on any available I/O pin of any port.  These are
              the connections used for this program:

                 -----------                   ----------
                | ATmega328 |                 |   LCD    |
                |           |                 |          |
                |        PD7|---------------->|D7        |
                |        PD6|---------------->|D6        |
                |        PD5|---------------->|D5        |
                |        PD4|---------------->|D4        |
                |           |                 |D3        |
                |           |                 |D2        |
                |           |                 |D1        |
                |           |                 |D0        |
                |           |                 |          |
                |        PB1|---------------->|E         |
                |        PB2|---------------->|RW        |
                |        PB0|---------------->|RS        |
                 -----------                   ----------

  **************************************************************************/

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "sdkconfig.h"

#include "project.h"

// LCD module information
// #define lcd_LineOne     0x00                    // start of line 1
// #define lcd_LineTwo     0x40                    // start of line 2
//#define   lcd_LineThree   0x14                  // start of line 3 (20x4)
//#define   lcd_lineFour    0x54                  // start of line 4 (20x4)
//#define   lcd_LineThree   0x10                  // start of line 3 (16x4)
//#define   lcd_lineFour    0x50                  // start of line 4 (16x4)

// LCD instructions
// #define lcd_Clear           0b00000001          // replace all characters with ASCII 'space'
// #define lcd_Home            0b00000010          // return cursor to first position on first line
// #define lcd_EntryMode       0b00000110          // shift cursor from left to right on read/write
// #define lcd_DisplayOff      0b00001000          // turn display off
// #define lcd_DisplayOn       0b00001111          // display on, cursor off, don't blink character
// // #define lcd_FunctionReset   0b00110000          // reset the LCD
// #define lcd_FunctionReset   0b00101000          // reset the LCD
// // #define lcd_FunctionSet4bit 0b00101000          // 4-bit data, 2-line display, 5 x 7 font
// #define lcd_FunctionSet4bit 0b00101100          // 4-bit data, 2-line display, 5 x 7 font
//#define lcd_FunctionSet4bit 0b00101000
//#define lcd_SetCursor       0b0000110100          // set cursor position

// Program ID
uint8_t empty_line[] = "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";

// Function Prototypes
static void lcd_write_4bit(uint8_t);
static void lcd_print_char(uint8_t);
static void lcd_check_BF_4(void);

void init_lcd(void)
{
    gpio_pad_select_gpio(PIN_LCD_D7);
    gpio_pad_select_gpio(PIN_LCD_D6);
    gpio_pad_select_gpio(PIN_LCD_D5);
    gpio_pad_select_gpio(PIN_LCD_D4);
    gpio_pad_select_gpio(PIN_LCD_E);
    gpio_pad_select_gpio(PIN_LCD_RS);
    gpio_pad_select_gpio(PIN_LCD_RW);

    gpio_set_direction(PIN_LCD_D7, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_LCD_D6, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_LCD_D5, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_LCD_D4, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_LCD_E, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_LCD_RS, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_LCD_RW, GPIO_MODE_OUTPUT);

    vTaskDelay(500 / portTICK_PERIOD_MS);
    gpio_set_level(PIN_LCD_RS, 0);
    gpio_set_level(PIN_LCD_E, 0);
    gpio_set_level(PIN_LCD_RW, 0);

    lcd_write_4bit(0b00110000);
    vTaskDelay(10 / portTICK_PERIOD_MS);

    lcd_write_4bit(0b00110000);
    vTaskDelay(10 / portTICK_PERIOD_MS);

    lcd_write_4bit(0b00110000);
    vTaskDelay(10 / portTICK_PERIOD_MS);

    lcd_write_4bit(0b00110000);
    vTaskDelay(10 / portTICK_PERIOD_MS);
    lcd_write_instruction(0b00110010);
    vTaskDelay(10 / portTICK_PERIOD_MS);

    lcd_write_instruction(0b00001000);
    vTaskDelay(10 / portTICK_PERIOD_MS);
    lcd_write_instruction(0b00000001);
    vTaskDelay(10 / portTICK_PERIOD_MS);
    lcd_write_instruction(0b00000110);
    vTaskDelay(10 / portTICK_PERIOD_MS);

    lcd_write_instruction(0b00001111);
    vTaskDelay(10 / portTICK_PERIOD_MS);
}

void lcd_print(uint8_t theString[])
{
    gpio_set_level(PIN_LCD_RW, 0);
    gpio_set_level(PIN_LCD_RS, 1);
    gpio_set_level(PIN_LCD_E, 0);
    volatile int i = 0;                             // character counter*/
    while (theString[i] != 0)
    {
        // lcd_check_BF_4();                           // make sure LCD controller is ready
        lcd_print_char(theString[i]);
        i++;
    }
    // gpio_set_level(RW_PIN, 1);
}

static void lcd_print_char(uint8_t theData)
{
    gpio_set_level(PIN_LCD_RW, 0);
    gpio_set_level(PIN_LCD_RS, 1);
    gpio_set_level(PIN_LCD_E, 0);

    lcd_write_4bit(theData);                           // write the upper 4-bits of the data
    lcd_write_4bit(theData << 4);                      // write the lower 4-bits of the data
}

void lcd_write_instruction(uint8_t theInstruction)
{
    gpio_set_level(PIN_LCD_RW, 0);
    gpio_set_level(PIN_LCD_RS, 0);
    gpio_set_level(PIN_LCD_E, 0);

    lcd_write_4bit(theInstruction);                    // write the upper 4-bits of the data
    lcd_write_4bit(theInstruction << 4);               // write the lower 4-bits of the data
    // gpio_set_level(RW_PIN, 1);
}

static void lcd_write_4bit(uint8_t theByte)
{

    gpio_set_level(PIN_LCD_D7, 0);
    if (theByte & 1<<7) {
      gpio_set_level(PIN_LCD_D7, 1);
    }
    gpio_set_level(PIN_LCD_D6, 0);
    if (theByte & 1<<6) {
      gpio_set_level(PIN_LCD_D6, 1);
    }
    gpio_set_level(PIN_LCD_D5, 0);
    if (theByte & 1<<5) {
      gpio_set_level(PIN_LCD_D5, 1);
    }
    gpio_set_level(PIN_LCD_D4, 0);
    if (theByte & 1<<4) {
      gpio_set_level(PIN_LCD_D4, 1);
    }

    // write the data
    gpio_set_level(PIN_LCD_E, 1);
    vTaskDelay(10 / portTICK_PERIOD_MS);
    gpio_set_level(PIN_LCD_E, 0);
    vTaskDelay(10 / portTICK_PERIOD_MS);
}

void lcd_clear(void) {
    lcd_write_instruction(0b00000001);
}

void lcd_clear_line1(void) {
    lcd_write_instruction(0b10000000);
    lcd_print(empty_line);
    lcd_write_instruction(0b10000000);
}

void lcd_clear_line2(void) {
    lcd_write_instruction(0b11000000);
    lcd_print(empty_line);
    lcd_write_instruction(0b11000000);
}

void lcd_go_to_line1(void) {
    lcd_write_instruction(0b10000000);
}

void lcd_go_to_line2(void) {
    lcd_write_instruction(0b11000000);
}

static void lcd_check_BF_4(void)
{
  vTaskDelay(100 / portTICK_PERIOD_MS);
//     uint8_t busy_flag_copy;                         // busy flag 'mirror'
//
//     // lcd_D7_ddr &= ~(1<<lcd_D7_bit);                 // set D7 data direction to input
//     // lcd_RS_port &= ~(1<<lcd_RS_bit);                // select the Instruction Register (RS low)
//     // lcd_RW_port |= (1<<lcd_RW_bit);                 // read from LCD module (RW high)
//     gpio_set_direction(D7_PIN, GPIO_MODE_INPUT);
//     gpio_set_level(RS_PIN, 0);
//     gpio_set_level(RW_PIN, 1);
//
//     do
//     {
//         busy_flag_copy = 0;                         // initialize busy flag 'mirror'
//         // lcd_E_port |= (1<<lcd_E_bit);               // Enable pin high
//         gpio_set_level(E_PIN, 1);
//         // _delay_us(1);                               // implement 'Delay data time' (160 nS) and 'Enable pulse width' (230 nS)
//         vTaskDelay(10 / portTICK_PERIOD_MS);
//
//         // busy_flag_copy |= (lcd_D7_pin & (1<<lcd_D7_bit));  // get actual busy flag status
//         busy_flag_copy |= gpio_get_level(D7_PIN);
//
//         // lcd_E_port &= ~(1<<lcd_E_bit);              // Enable pin low
//         gpio_set_level(E_PIN, 0);
//         // _delay_us(1);                               // implement 'Address hold time' (10 nS), 'Data hold time' (10 nS), and 'Enable cycle time' (500 nS )
//         vTaskDelay(10 / portTICK_PERIOD_MS);
//
// // read and discard alternate nibbles (D3 information)
//         // lcd_E_port |= (1<<lcd_E_bit);               // Enable pin high
//         gpio_set_level(E_PIN, 1);
//         // _delay_us(1);                               // implement 'Delay data time' (160 nS) and 'Enable pulse width' (230 nS)
//         vTaskDelay(10 / portTICK_PERIOD_MS);
//         // lcd_E_port &= ~(1<<lcd_E_bit);              // Enable pin low
//         gpio_set_level(E_PIN, 0);
//         // _delay_us(1);                               // implement 'Address hold time (10 nS), 'Data hold time' (10 nS), and 'Enable cycle time' (500 nS )
//         vTaskDelay(10 / portTICK_PERIOD_MS);
//
//     } while (busy_flag_copy);                       // check again if busy flag was high
//
// // arrive here if busy flag is clear -  clean up and return
//     // lcd_RW_port &= ~(1<<lcd_RW_bit);                // write to LCD module (RW low)
//     // lcd_D7_ddr |= (1<<lcd_D7_bit);                  // reset D7 data direction to output
//     gpio_set_level(RW_PIN, 0);
//     gpio_set_direction(D7_PIN, GPIO_MODE_OUTPUT);
}
