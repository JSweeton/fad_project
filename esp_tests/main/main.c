#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_system.h"
#include "esp_log.h"

#include "driver/uart.h"

const char TEST_TAG[] = "FAD_TEST";
QueueHandle_t uart_handle;


int write_to_uart(char *data) {
    int len = strlen(data);
    int written_bytes = 0;
    if(len > 0) {
       written_bytes = uart_write_bytes(UART_NUM_0, data, len);
    }
    
    return written_bytes;
}
void app_main(void) {

    const int uart_num = UART_NUM_0;
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    // Configure UART parameters
    ESP_ERROR_CHECK(uart_param_config(uart_num, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_NUM_0, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_driver_install(uart_num, 256, 256, 10, &uart_handle, 0));
    ESP_LOGI(TEST_TAG, "Beginning initialization");
    char *in_buffer = (char *)malloc(50 * sizeof(char));
    char *out_buffer = (char *)malloc(50 * sizeof(char));
    assert(in_buffer);
    // ESP_LOGI(TEST_TAG, "Successfully initialized");
    assert(out_buffer);
    ESP_LOGI(TEST_TAG, "Successfully initialized");

    //give some cool down time
    vTaskDelay(100);
    int read_bytes = uart_read_bytes(uart_num, in_buffer, 45, 100);
    
    char output[50];

    if(strlen(in_buffer) > 0) {
        sprintf(output, "0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x", 
                        in_buffer[0], in_buffer[1], in_buffer[2], in_buffer[3], in_buffer[4], in_buffer[5], in_buffer[6], in_buffer[7]);
        ESP_LOGI(TEST_TAG, "Read msg, num: %s", output);
    } 

    ESP_LOGI(TEST_TAG, "Attempted read");
    out_buffer[0] = 'y';
    char msg[50];   
    // sprintf(msg, "This is my test. Please hear my prayers.\n");
    // int written_bytes = write_to_uart(msg);
    // ESP_LOGI(TEST_TAG, "Wrote %d bytes", written_bytes);



    // int written_bytes = 0;
    // while(1) {
    //     vTaskDelay(10);
    //     int read_bytes = uart_read_bytes(uart_num, in_buffer, 1, 10);

    //     if (read_bytes != -1) {
    //         // out_buffer[0] = in_buffer[0];
    //         ESP_LOGI(TEST_TAG, "Read a byte!");
    //         // written_bytes = uart_write_bytes(uart_num, out_buffer, 1);
    //     }
    // }
}
