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
#include "algo_white.h"

#include "driver/uart.h"

#define BUF_SIZE (1024)
#define RD_BUF_SIZE (512)
#define WR_BUF_SIZE (512)

const char SERIAL_TAG[] = "FAD_SERIAL";
const char PACKET_HEADER[] = "DATA\n";
const char PACKET_FOOTER[] = "\nENDSIG\n";
const char STOP_MSG[] = "STOPSIG\n";
const int STOP_MSG_LENGTH = 8;

QueueHandle_t uart_handle;
QueueHandle_t uart_write_handle;
const int uart_num = UART_NUM_0;

typedef enum {
    SEND_PACKET = 0,
    SEND_MSG,
    SEND_STOP,
    SEND_GENERIC,
    SEND_TEST,
} write_cmd;

typedef struct uart_write_evt_t {
    write_cmd cmd;
    char data[128];
    int size;
} uart_write_evt_t;

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
    err = uart_driver_install(uart_num, RD_BUF_SIZE, WR_BUF_SIZE, 10, &uart_handle, 0);

    ESP_ERROR_CHECK(err);

}

char last_line[128] = "";

int split_str_by_eol(char **dest, char *source, int length) {
    dest[0] = source;
    int tot = 1;
    for (int i = 0; i < length-1; i++) {
        if (source[i] == '\n') {
            dest[tot] = source + i + 1;
            tot++;
        }
    } 

    if (*last_line != 0) { // Check to see if there is an unfinished line
        strncat(last_line, dest[0], dest[1] - dest[0] - 1);
    }
    dest[tot] = 0;
    memset(last_line, 0, sizeof(last_line));
    if (source[length-1] != '\n') {
        strcpy(last_line, dest[tot-1]);
        return -1;
    }

    return tot;
}


void handle_serial_input(char *line, int length) {
    // Scan through to find end line characters and split up input based on those values
    // char** strings = (char **) malloc(sizeof(char**) * 10);
    // int string_amount = split_str_by_eol(strings, line, length);
    uart_write_evt_t event;
    event.cmd = SEND_MSG;
    sprintf(event.data, "line");
    event.size = 30;
    xQueueSend(uart_write_handle, (void *)&event, (portTickType) portMAX_DELAY);

}

void serial_read_task(void *params) {

    uart_event_t event;

    for(;;) {
        if (xQueueReceive(uart_handle, (void *)&event, (portTickType)portMAX_DELAY)) {
            switch (event.type) {

                case UART_DATA: ;
                    char *data = (char *) malloc(sizeof(char) * event.size + 1);
                    data[event.size] = 0;
                    uart_read_bytes(uart_num, data, event.size, 2);
                    handle_serial_input(data, event.size);
                    free(data);
                    data = NULL;
                    break;

                case UART_BUFFER_FULL:
                    ESP_LOGI(SERIAL_TAG, "[BUFFER_FULL]:");
                    break;
                
                default:
                    ESP_LOGI(SERIAL_TAG, "[NOT HANDLED]: %d", event.type);
            }
        }
    }
    
    vTaskDelete(NULL);
}

void serial_write_task(void * params) {
    /* TODO: Handle task of writing to serial given data */
    uart_write_evt_t event;  

    for (;;) {
        if ( xQueueReceive(uart_write_handle, (void *)&event, (portTickType)portMAX_DELAY) ) {
            /* TODO: Parse event, send data accordingly */

            switch (event.cmd) {
                /* uart_write_bytes copies data to TX ring bufffer, possibly blocking until there is space */
                case SEND_PACKET:
                    uart_write_bytes(uart_num, PACKET_HEADER, 5);
                    uart_write_bytes(uart_num, "\x00\x10\n", 3);
                    uart_write_bytes(uart_num, (void *)event.data, event.size);
                    uart_write_bytes(uart_num, PACKET_FOOTER, 8);
                    break;
                case SEND_MSG:
                    ESP_LOGI(SERIAL_TAG, "%s", event.data);
                    break;
                case SEND_STOP:
                    uart_write_bytes(uart_num, STOP_MSG, STOP_MSG_LENGTH);
                    break;
                case SEND_GENERIC:
                    uart_write_bytes(uart_num, event.data, event.size);
                    break;
                case SEND_TEST:
                    ESP_LOGI(SERIAL_TAG, "TEST");
                default:
                    break;
            }
        } 
    }
}

void app_main(void) {

    init_uart_0(115200);

    TaskHandle_t uart_read_task, uart_write_task;
    uart_write_handle = xQueueCreate(10, sizeof(uart_write_evt_t));
    xTaskCreate(serial_read_task, "Uart0 Queue Task", 2048, NULL, configMAX_PRIORITIES - 3, &uart_read_task);
    xTaskCreate(serial_write_task, "Uart0 Write Task", 2048, NULL, configMAX_PRIORITIES - 3, &uart_write_task);

}
