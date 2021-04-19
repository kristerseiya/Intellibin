
#include "driver/uart.h"
#include "esp_log.h"
#include "project.h"

void init_uart(void) {

  uart_config_t uart_config = {
    .baud_rate = 115200,
    .data_bits = UART_DATA_8_BITS,
    .parity = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    // .flow_ctrl = UART_HW_FLOWCTRL_CTS_RTS,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    .rx_flow_ctrl_thresh = 122,
  };
  ESP_ERROR_CHECK(uart_param_config(UART_NUM_2, &uart_config));

  ESP_ERROR_CHECK(uart_set_pin(UART_NUM_2, PIN_UART_TX, PIN_UART_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

  // Setup UART buffered IO with event queue
const int uart_buffer_size = (1024 * 2);
QueueHandle_t uart_queue;
// Install UART driver using an event queue here
ESP_ERROR_CHECK(uart_driver_install(UART_NUM_2, uart_buffer_size,
                                    uart_buffer_size, 10, &uart_queue, 0));
}

void uart_send(const char* str, size_t size) {
  uart_write_bytes(UART_NUM_2, str, size);
  printf("%d: %s", size, str);
}
