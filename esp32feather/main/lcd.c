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

// #define F_CPU 16000000UL

// #include <avr/io.h>
// #include <util/delay.h>

// LCD interface (should agree with the diagram above)
// #define lcd_D7_port     PORTD                   // lcd D7 connection
// #define lcd_D7_bit      PORTD7
// #define lcd_D7_ddr      DDRD
// #define lcd_D7_pin      PIND                    // busy flag
//
// #define lcd_D6_port     PORTD                   // lcd D6 connection
// #define lcd_D6_bit      PORTD6
// #define lcd_D6_ddr      DDRD
//
// #define lcd_D5_port     PORTD                   // lcd D5 connection
// #define lcd_D5_bit      PORTD5
// #define lcd_D5_ddr      DDRD
//
// #define lcd_D4_port     PORTD                   // lcd D4 connection
// #define lcd_D4_bit      PORTD4
// #define lcd_D4_ddr      DDRD
//
// #define lcd_E_port      PORTB                   // lcd Enable pin
// #define lcd_E_bit       PORTB1
// #define lcd_E_ddr       DDRB
//
// #define lcd_RS_port     PORTB                   // lcd Register Select pin
// #define lcd_RS_bit      PORTB0
// #define lcd_RS_ddr      DDRB
//
// #define lcd_RW_port     PORTB                   // lcd Read/Write pin
// #define lcd_RW_bit      PORTB2
// #define lcd_RW_ddr      DDRB

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "sdkconfig.h"

#include "project.h"

#define D7_PIN  GPIO_NUM_14
#define D6_PIN  GPIO_NUM_32
#define D5_PIN  GPIO_NUM_15
#define D4_PIN  GPIO_NUM_33
#define E_PIN   GPIO_NUM_27
#define RW_PIN  GPIO_NUM_12
#define RS_PIN  GPIO_NUM_13

// LCD module information
#define lcd_LineOne     0x00                    // start of line 1
#define lcd_LineTwo     0x40                    // start of line 2
//#define   lcd_LineThree   0x14                  // start of line 3 (20x4)
//#define   lcd_lineFour    0x54                  // start of line 4 (20x4)
//#define   lcd_LineThree   0x10                  // start of line 3 (16x4)
//#define   lcd_lineFour    0x50                  // start of line 4 (16x4)

// LCD instructions
#define lcd_Clear           0b00000001          // replace all characters with ASCII 'space'
#define lcd_Home            0b00000010          // return cursor to first position on first line
#define lcd_EntryMode       0b00000110          // shift cursor from left to right on read/write
#define lcd_DisplayOff      0b00001000          // turn display off
#define lcd_DisplayOn       0b00001111          // display on, cursor off, don't blink character
// #define lcd_FunctionReset   0b00110000          // reset the LCD
#define lcd_FunctionReset   0b00101000          // reset the LCD
// #define lcd_FunctionSet4bit 0b00101000          // 4-bit data, 2-line display, 5 x 7 font
#define lcd_FunctionSet4bit 0b00101100          // 4-bit data, 2-line display, 5 x 7 font
//#define lcd_FunctionSet4bit 0b00101000
//#define lcd_SetCursor       0b0000110100          // set cursor position

// Program ID
uint8_t recyclable_string[]   = "recyclable";
uint8_t capacity_string[]  = "100% full";
uint8_t imagproc[]     = "Image Processing";
uint8_t compimag[]     = "Computational Imaging";
uint8_t mes1[] = "ok";
uint8_t mes2[] = "what time?";

// Function Prototypes
void lcd_write_4(uint8_t);
void lcd_write_instruction_4f(uint8_t);
void lcd_write_character_4f(uint8_t);
void lcd_write_string_4f(uint8_t *);
void lcd_init_4f(void);
void lcd_check_BF_4(void);

/******************************* Main Program Code *************************/
void init_lcd(void)
{
// configure the microprocessor pins for the data lines
    // lcd_D7_ddr |= (1<<lcd_D7_bit);                  // 4 data lines - output
    // lcd_D6_ddr |= (1<<lcd_D6_bit);
    // lcd_D5_ddr |= (1<<lcd_D5_bit);
    // lcd_D4_ddr |= (1<<lcd_D4_bit);
    gpio_pad_select_gpio(D7_PIN);
    gpio_pad_select_gpio(D6_PIN);
    gpio_pad_select_gpio(D5_PIN);
    gpio_pad_select_gpio(D4_PIN);
    gpio_pad_select_gpio(E_PIN);
    gpio_pad_select_gpio(RS_PIN);
    gpio_pad_select_gpio(RW_PIN);

    gpio_set_direction(D7_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(D6_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(D5_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(D4_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(E_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(RS_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(RW_PIN, GPIO_MODE_OUTPUT);

    // gpio_set_pull_mode(D7_PIN, GPIO_PULLDOWN_ONLY);
    // gpio_set_pull_mode(D6_PIN, GPIO_PULLDOWN_ONLY);
    // gpio_set_pull_mode(D5_PIN, GPIO_PULLDOWN_ONLY);
    // gpio_set_pull_mode(D4_PIN, GPIO_PULLDOWN_ONLY);
    // gpio_set_pull_mode(E_PIN, GPIO_PULLDOWN_ONLY);
    // gpio_set_pull_mode(RS_PIN, GPIO_PULLDOWN_ONLY);
    // gpio_set_pull_mode(RW_PIN, GPIO_PULLDOWN_ONLY);

//     gpio_set_level(D7_PIN, 1);
//     gpio_set_level(D6_PIN, 1);
//     gpio_set_level(D5_PIN, 1);
//     gpio_set_level(D4_PIN, 1);

    lcd_init_4f();
    // lcd_write_string_4f(mes1);
    //
    // lcd_write_instruction_4f(0b11000000);
    // vTaskDelay(10 / portTICK_PERIOD_MS);
    //
    // lcd_write_string_4f(mes2);
// endless loop
    while(1) {
      vTaskDelay(10);
    }
    // return 0;
}
/******************************* End of Main Program Code ******************/

/*============================== 4-bit LCD Functions ======================*/
/*
  Name:     lcd_init_4f
  Purpose:  initialize the LCD module for a 4-bit data interface
  Entry:    equates (LCD instructions) set up for the desired operation
  Exit:     no parameters
  Notes:    uses the busy flag instead of time delays when possible
*/
void lcd_init_4f(void)
{
    vTaskDelay(500 / portTICK_PERIOD_MS);
    gpio_set_level(RS_PIN, 0);
    gpio_set_level(E_PIN, 0);
    gpio_set_level(RW_PIN, 0);

    lcd_write_4(0b00110000);
    vTaskDelay(10 / portTICK_PERIOD_MS);

    lcd_write_4(0b00110000);
    vTaskDelay(10 / portTICK_PERIOD_MS);

    lcd_write_4(0b00110000);
    vTaskDelay(10 / portTICK_PERIOD_MS);

    lcd_write_4(0b00110000);
    vTaskDelay(10 / portTICK_PERIOD_MS);
    lcd_write_instruction_4f(0b00110010);
    vTaskDelay(10 / portTICK_PERIOD_MS);

    lcd_write_instruction_4f(0b00001000);
    vTaskDelay(10 / portTICK_PERIOD_MS);
    lcd_write_instruction_4f(0b00000001);
    vTaskDelay(10 / portTICK_PERIOD_MS);
    lcd_write_instruction_4f(0b00000110);
    vTaskDelay(10 / portTICK_PERIOD_MS);

    lcd_write_instruction_4f(0b00001111);
    vTaskDelay(10 / portTICK_PERIOD_MS);
}

/*...........................................................................
  Name:     lcd_write_string_4f
; Purpose:  display a string of characters on the LCD
  Entry:    (theString) is the string to be displayed
  Exit:     no parameters
  Notes:    uses the busy flag instead of time delays
*/
void lcd_write_string_4f(uint8_t theString[])
{
    gpio_set_level(RW_PIN, 0);
    gpio_set_level(RS_PIN, 1);
    gpio_set_level(E_PIN, 0);
    volatile int i = 0;                             // character counter*/
    while (theString[i] != 0)
    {
        // lcd_check_BF_4();                           // make sure LCD controller is ready
        lcd_write_character_4f(theString[i]);
        i++;
        //vTaskDelay(80 / portTICK_PERIOD_MS);
    }
    // gpio_set_level(RW_PIN, 1);
}

/*...........................................................................
  Name:     lcd_write_character_4f
  Purpose:  send a byte of information to the LCD data register
  Entry:    (theData) is the information to be sent to the data register
  Exit:     no parameters
  Notes:    configures RW (busy flag is implemented)
*/
void lcd_write_character_4f(uint8_t theData)
{
    gpio_set_level(RW_PIN, 0);
    gpio_set_level(RS_PIN, 1);
    gpio_set_level(E_PIN, 0);

    lcd_write_4(theData);                           // write the upper 4-bits of the data
    lcd_write_4(theData << 4);                      // write the lower 4-bits of the data
}

/*...........................................................................
  Name:     lcd_write_instruction_4f
  Purpose:  send a byte of information to the LCD instruction register
  Entry:    (theInstruction) is the information to be sent to the instruction register
  Exit:     no parameters
  Notes:    configures RW (busy flag is implemented)
*/
void lcd_write_instruction_4f(uint8_t theInstruction)
{
    gpio_set_level(RW_PIN, 0);
    gpio_set_level(RS_PIN, 0);
    gpio_set_level(E_PIN, 0);

    lcd_write_4(theInstruction);                    // write the upper 4-bits of the data
    lcd_write_4(theInstruction << 4);               // write the lower 4-bits of the data
    // gpio_set_level(RW_PIN, 1);
}

/*...........................................................................
  Name:     lcd_write_4
  Purpose:  send a nibble (4-bits) of information to the LCD module
  Entry:    (theByte) contains a byte of data with the desired 4-bits in the upper nibble
            RS is configured for the desired LCD register
            E is low
            RW is low
  Exit:     no parameters
  Notes:    use either time delays or the busy flag
*/
void lcd_write_4(uint8_t theByte)
{

    gpio_set_level(D7_PIN, 0);
    if (theByte & 1<<7) {
      gpio_set_level(D7_PIN, 1);
    }
    gpio_set_level(D6_PIN, 0);
    if (theByte & 1<<6) {
      gpio_set_level(D6_PIN, 1);
    }
    gpio_set_level(D5_PIN, 0);
    if (theByte & 1<<5) {
      gpio_set_level(D5_PIN, 1);
    }
    gpio_set_level(D4_PIN, 0);
    if (theByte & 1<<4) {
      gpio_set_level(D4_PIN, 1);
    }

    // write the data
    gpio_set_level(E_PIN, 1);
    vTaskDelay(10 / portTICK_PERIOD_MS);
    gpio_set_level(E_PIN, 0);
    vTaskDelay(10 / portTICK_PERIOD_MS);
}

/*...........................................................................
  Name:     lcd_check_BF_4
  Purpose:  check busy flag, wait until LCD is ready
  Entry:    no parameters
  Exit:     no parameters
  Notes:    main program will hang if LCD module is defective or missing
            data is read while 'E' is high
            both nibbles must be read even though desired information is only in the high nibble
*/
void lcd_check_BF_4(void)
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
