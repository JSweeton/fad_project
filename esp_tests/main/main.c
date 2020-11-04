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

#define BUF_SIZE (1024)
#define RD_BUF_SIZE (BUF_SIZE)

const char TEST_TAG[] = "FAD_TEST";
QueueHandle_t uart_handle;
const int uart_num = UART_NUM_0;


int write_to_uart(char *data) {
    int len = strlen(data);
    int written_bytes = 0;
    if(len > 0) {
       written_bytes = uart_write_bytes(UART_NUM_0, data, len);
    }
    
    return written_bytes;

}

void init_uart_0(int baud_rate) {
    
    uart_config_t uart_config = {
        .baud_rate = baud_rate,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };

    // Configure UART parameters
    esp_err_t err = uart_param_config(uart_num, &uart_config);
    err = uart_set_pin(uart_num, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    err = uart_driver_install(uart_num, 256, 256, 10, &uart_handle, 0);

    ESP_ERROR_CHECK(err);

}

void serial_read_task(void *params) {

    char *data = (char *)malloc(sizeof(char) * RD_BUF_SIZE);
    uart_event_t event;

    for(;;) {
        if (xQueueReceive(uart_handle, (void *)&event, (portTickType)portMAX_DELAY)) {
            bzero(data, RD_BUF_SIZE);
            switch (event.type) {

                case UART_DATA:
                    uart_read_bytes(uart_num, data, event.size, 2);
                    ESP_LOGI(TEST_TAG, "[DATA]: %s", data);
                    break;

                case UART_BUFFER_FULL:
                    ESP_LOGI(TEST_TAG, "[BUFFER_FULL]:");
                    break;
                
                default:
                    ESP_LOGI(TEST_TAG, "[NOT HANDLED]: %d", event.type);
            }
        }
    }
    
    free(data);
    data = NULL;
    vTaskDelete(NULL);
}
void app_main(void) {

    init_uart_0(115200);

    TaskHandle_t uart_read_task;
    xTaskCreate(serial_read_task, "Uart0 Queue Task", 256, NULL, configMAX_PRIORITIES - 3, &uart_read_task);
}
