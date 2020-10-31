#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_system.h"
#include "esp_log.h"

#include "driver/uart.h"

const char TEST_TAG[] = "FAD_TEST";

void app_main(void) {
    ESP_LOGI(TEST_TAG, "Configuring UART parameters");

    const int uart_num = UART_NUM_0;
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_CTS_RTS,
        .rx_flow_ctrl_thresh = 122,
    };
    // Configure UART parameters
    ESP_ERROR_CHECK(uart_param_config(uart_num, &uart_config));
    char *buffer = malloc(50 * sizeof(char));
    while(1) {
        int read_bytes = uart_read_bytes(uart_num, buffer, 1, 10);
        if(buffer[0] > 0) {
           if(uart_write_bytes(uart_num, buffer, read_bytes) == -1) {
           }
        }
    }
}
