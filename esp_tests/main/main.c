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
    // ESP_LOGI(TEST_TAG, "Configuring UART parameters");

    char *buffer = (char *)malloc(50 * sizeof(char));
    int written_bytes = 0;

    while(1) {
        vTaskDelay(10);
        int read_bytes = uart_read_bytes(uart_num, buffer, 1, 10);

        if (read_bytes != -1) {
            written_bytes = uart_write_bytes(uart_num, buffer, 1);
        }
    }
}
