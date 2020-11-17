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
#define PACKET_DATA_SIZE 256 
#define PACKET_TOTAL_SIZE (PACKET_DATA_SIZE + 16)

const char SERIAL_TAG[] = "FAD_SERIAL";
const char PACKET_HEADER[] = "DATA\0";
const char PACKET_FOOTER[] = "ENDSIG\0\n";
const char STOP_MSG[] = "STOPSIG\n";
const int STOP_MSG_LENGTH = 8;

char packet_buffer[PACKET_TOTAL_SIZE + 1] = "";
char buffer_pos = 0;

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
    char *data;
    int size;
} uart_write_evt_t;

typedef struct packet {
  char header[5];
  char length[3];
  char data[PACKET_DATA_SIZE];
  char footer[8];
} packet;

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

void handle_packet(packet *p)
{
    ESP_LOGI(SERIAL_TAG, "Handling packet!");
}

/* For now, only expect to handle data packets */

void handle_serial_input(char *line, int line_length)
{
    ESP_LOGI(SERIAL_TAG, "HANDLING");
    int pos = 0; // Points to the last EOL character position
    char last_string_pos = 0;
    while (pos < line_length)
    {
        if (line[pos] == '\n')
        {
            // In this case, attempt to process last piece of data as packet
            if (pos < PACKET_TOTAL_SIZE - 1)
            {
                // In this case, we received the end of a line started in a
                // previous call to the serial input handler
                memcpy(packet_buffer + buffer_pos, line, pos + 1);
            }
            else
            {
                if (pos - last_string_pos == PACKET_TOTAL_SIZE - 1)
                {
                    // Here, the width of the chunk matches the expected packet width.
                    // Copy the data into the packet buffer
                    memcpy(packet_buffer, line + last_string_pos, PACKET_TOTAL_SIZE);
                }
            }
            // Verify that the packet ends correctly
            if (packet_buffer[PACKET_TOTAL_SIZE - 1] == '\n')
            {
                handle_packet((packet *)packet_buffer);
                memset(packet_buffer, 0, PACKET_TOTAL_SIZE + 1);
                buffer_pos = 0;
            }
            last_string_pos = pos + 1;
        }
        pos++;
    }

    memcpy(packet_buffer + buffer_pos, line + last_string_pos, pos - last_string_pos);

    if (buffer_pos > 0 && strcmp(packet_buffer, "DATA") != 0)
    {
        // If the beginning of the packet buffer doesn't read like the start of a packet, then the data is corrupted
        buffer_pos = 0;
    }
    else
    {
        buffer_pos += pos - last_string_pos;
    }
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
                    // handle_serial_input(data, event.size);
                    uart_write_bytes(uart_num, data, event.size);
                    // ESP_LOGI(SERIAL_TAG, "DATA AMOUNT: %d", data[event.size - 1]);
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
                    char lengths[3] = {event.size >> 2, event.size, 0};
                    uart_write_bytes(uart_num, lengths, 3);
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

            if (event.data != NULL) free(event.data);
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
