/**
 * main.c
 * Author: Corey Bean
 * 
 * This program allows an ESP32 to accept packets off of a serial line, perform an
 * FAD algorithm on the data, and return packets over the serial line with the output
 * of the algorithm. A packet is of the form:
 * HEADER:  The string "DATA" as chars, then a single 0 ('\0'), then a one byte value indicating packets 
 *          to follow, then a one byte value indicating number of packets sent previously in the chunk
 * DATA:    256 Bytes of raw bytes data
 * FOOTER:  The string "ENDSIG" as chars, then a single 0 ('\0'), then a newline character ('\n' or '\x0a)
 * 
 * This program utilizes the serial UART library from Espressif. After initialization, this library
 * provides serial data to a queue, handled in this program by the serial_read_task. This data is then
 * sent to a handle_serial_input function, which parses lines for packets and sends packets to a packet
 * queue. In this queue, the program parses the packet data and behaves accordingly; most of the time, it
 * will send the data to an FAD function and return output data through the packet sender in the uart_write_task.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/uart.h"

#include "algo_test.h"
#include "fad_defs.h"
#include "algo_white.h"

#define BUF_SIZE (1024)
#define RD_BUF_SIZE (1024)
#define WR_BUF_SIZE (1024)
#define PACKET_DATA_SIZE 256
#define PACKET_TOTAL_SIZE (PACKET_DATA_SIZE + 16)
#define UART_INSTANCE UART_NUM_0

enum write_cmd {
    SEND_PACKET = 0,    /* Send data as a packet */
    SEND_MSG,           /* Send data as ESP_LOG */
    SEND_STOP,          /* Send STOP message */
    SEND_GENERIC,       /* Send data string as straight serial data */
    SEND_TEST,          /* Test cmd for bug testing */
};

typedef struct uart_write_evt_t {
    enum write_cmd cmd;     /* The commands for the uart write handler. */
    uint8_t *data;          /* The data to be written to the handler */
    int size;               /* The size, in bytes, of the data to be copied */
    char num_to_follow;     /* For a packet, number of packets that are coming after */
    char error_id;          /* For a packet, number of packets that came before */
} uart_write_evt_t;

typedef struct packet {
    char header[5];                 /* Data header, as described in intro */
    char type[3];                   /* Type, two-letter string like "U1" or "U2" */
    char data[PACKET_DATA_SIZE];    /* The bytes of data as raw bytes */
    uint16_t packets_incoming;      /* The number of packets planned to follow */
    uint16_t packet_missed_id;      /* 0 if no errors, packets_incoming of missed packet if there is one */
    char footer[4];                 /* The footer of the data, e.g. "END/0/n" */
} packet;

uint8_t packet_buffer[PACKET_TOTAL_SIZE + 1] = "";  /* Holds the contents of partial packets */
int packet_buffer_pos = 0;  /* Tracks where to paste next into packet buffer */

QueueHandle_t uart_handle;
QueueHandle_t uart_write_handle;
QueueHandle_t packet_queue;
algo_func_t fad_algo;

const char SERIAL_TAG[] = "FAD_SERIAL";
const char PACKET_HEADER[] = "DATA\0";
const char PACKET_FOOTER[] = "END\n";
const char STOP_MSG[] = "STOPSIG\n";
const int STOP_MSG_LENGTH = 8;

/**
 * @brief Initialize the UART serial line with a standard serial configuration
 * @param baud_rate The desired baud rate for the serial line
 */
void init_uart_0(int baud_rate)
{
    uart_config_t uart_config = {
        .baud_rate = baud_rate,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };

    // Configure UART parameters. See Espressif's UART API for details
    esp_err_t err = uart_param_config(UART_INSTANCE, &uart_config);
    err = uart_set_pin(UART_INSTANCE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    err = uart_driver_install(UART_INSTANCE, RD_BUF_SIZE, WR_BUF_SIZE, 10, &uart_handle, 0);

    ESP_ERROR_CHECK(err);
}


/**
 * @brief Given a packet, send the packet to the queue, allocating memory as necessary.
 * 
 * @param p The packet to verify and send through the queue.
 */
void send_packet_to_queue(packet *p)
{
    if( strncmp(p->footer, "END", 3) == 0 ) /* Verify packet ends with the footer */
        xQueueSend( packet_queue, (void *)p, (portTickType)portMAX_DELAY );
    else ESP_LOGI(SERIAL_TAG, "Bad Packet noticed in send_packet_to_queue!");
}


/**
 * @brief Given data of size PACKET_DATA_SIZE, send data as a single packet through serial port.
 * 
 * @param data Pointer to the data to be sent. Will be freed through UART Write Queue
 * @param size The size of the data to be sent. Has to be 256 to work, meant to programatically confirm data size.
 */
void send_data_as_one_packet(uint8_t *data, int size, int num_to_follow, int error_id)
{
    if (data != NULL && size == PACKET_DATA_SIZE)
    {
        uart_write_evt_t msg;
        msg.cmd = SEND_PACKET;
        msg.size = size; 
        msg.data = data;
        msg.num_to_follow = num_to_follow;
        msg.error_id = error_id;

        xQueueSend(uart_write_handle, (void *)&msg, (portTickType)portMAX_DELAY);
    }
}


/**
 * @brief Finds and returns the position (as a pointer) of the string "DATA" 
 * in the input string. 
 * 
 * @param data A pointer to the data to search for the DATA phrase inside
 * @param size The number of chars to search through
 * 
 * @return  A pointer pointing to the data starting at the packet header
 *          NULL if packet header is not found 
 */
char *find_data_string(char *data, int size)
{
    int d_pos = 0; /* Tracks how many of the past letters have matched the PACKET_HEADER */

    for (int i = 0; i < size; i++)
    {
        if (data[i] == PACKET_HEADER[d_pos]) 
            d_pos = d_pos + 1; 
            else d_pos = 0;

        if (d_pos == 4) return &data[i - 3];    /* The past 4 letters have matched */
    }

    return NULL;
}


/**
 * @brief Parses serial input and checks for a packet, and handles a packet if a whole one is found.
 * @param data A pointer to the serial data
 * @param size The number of bytes to read/parse starting at data
 * @return True (1) or False (0) indicating whether we are in the middle of receiving a packet after the data is parsed 
 */
int handle_start_of_packet(char *data, int size)
{
    if (strncmp(data, PACKET_HEADER, size) == 0) /* Header of packet found at beginning! Should be the case most of the time. */
    {
        if (size < PACKET_TOTAL_SIZE) /* Only received part of a packet */
        {
            memcpy(packet_buffer, data, size);
            packet_buffer_pos = size;
            return 1;   /* return a 1 since we only have a partial packet in packet buffer */
        }
        else /* size is greater than that of one packet, received more than one. Will not happen in any case, but here just in case */
        {
            memcpy(packet_buffer, data, PACKET_TOTAL_SIZE);
            send_packet_to_queue((packet *)packet_buffer);
            return handle_start_of_packet(data + PACKET_TOTAL_SIZE, size - PACKET_TOTAL_SIZE); /* Send extra data back through handler */
        }
    }
    else /* strncmp != 0, packet doesn't begin with data */
    {
        char *data_string = find_data_string(data, size); // Returns first occurence of "DATA" in data
        if (data_string)
        {
            return handle_start_of_packet(data_string, size - (char)(data_string - data));
        }
        return 0;
    }
}


/**
 * @brief This function accepts and parses serial inputs in the case that we are in the middle of receiving a packet
 * @param data A pointer to the serial data
 * @param size The number of bytes to read/parse starting at data
 * @return True (1) or False (0) indicating whether we are still in the middle of receiving a packet 
 */
int continue_handling_packet(char *data, int size)
{
    int room_to_copy = PACKET_TOTAL_SIZE - packet_buffer_pos;
    if (size >= room_to_copy)
    {
        memcpy(packet_buffer + packet_buffer_pos, data, room_to_copy);
        send_packet_to_queue((packet *)packet_buffer);
        return handle_start_of_packet(data + room_to_copy, size - room_to_copy);
    }
    else    /* data won't fill to end of packet buffer */
    {
        memcpy(packet_buffer + packet_buffer_pos, data, size);
        packet_buffer_pos += size;
        return 1;
    }
}


/**
 * @brief This function takes a set amount of serial data and handles a packet. If we
 * weren't previously receiving a packet, then we check the line for the header "DATA".
 * If it exists, we place the line in the packet buffer global, and if it is full, we
 * handle the packet. If we were previously receiving a packet and it is unfinished,
 * we add the amount of data that can fit into the remaining space of the packet buffer.
 * 
 * @param data A pointer to the serial data to be processed
 * @param size The number of bytes of the serial data to be processed.
 * @param receiving_packet A true (1) or false (0) value indicating whether we are in the middle of a packet
 * @return  '1' if we are now in the middle of receiving a packet
 *          '0' if we are not in the middle of receiving a packet 
 */
int handle_serial_input(char *data, int size, int receiving_packet)
{
    if (size == 0)
        return 0;

    /* We are not in the middle of receiving a packet, so search this packet for header */
    if (!receiving_packet) 
        return handle_start_of_packet(data, size);     

    /* we are previously receiving packet, so check how much we have in buffer and copy into buffer */
    else 
        return continue_handling_packet(data, size);  
}


/**
 * @brief A FreeRTOS task that checks the packet queue for packets, parses the packets, 
 * and reacts according to the packet content. 
 */
void packet_handler_task(void *params)
{
    packet p;
    uint8_t *output_data = NULL;
    int num_sent = 0;

    for (;;)
    {
        if (xQueueReceive(packet_queue, (void *)&p, (portTickType)portMAX_DELAY) == pdPASS)
        {
           /* Incoming packets are formatted as sequential 16-bit values. Need two input packets to create 1 output packet */
            // ESP_LOGI(SERIAL_TAG, "HANDLING PACKET, remaining: %d", p.packets_incoming);

            int write_pos = PACKET_DATA_SIZE / 2;

            if (p.packets_incoming % 2 == 1)    /* This means this packet is the first of a pair (since "odd" packets have even packets left) */
            {
                output_data = (uint8_t *)malloc(PACKET_DATA_SIZE); /* Reallocate output data to a new address for fresh memory (prevents corrupting... */
                write_pos = 0;                                     /* ...previous packets before they've been written) */
            }

            // ESP_LOGI(SERIAL_TAG, "Beginning algo...");
            fad_algo((uint16_t *)p.data, output_data + write_pos, 0, 0, 1);

            if (write_pos == 128)   /* Packet is ready! Send out with appropriate data */
            {
                /* If the first packet comes in with 3 remaining, that means four packets are input, so 2 must be output (or 1 remaining) */
                /* If the first packet comes in with 7 remaining, that means 8 packets are input, so 4 must be output (or 3 remaining) */
                int num_to_follow = p.packets_incoming / 2; 
                uint16_t error_id = 0;
                send_data_as_one_packet(output_data, PACKET_DATA_SIZE, num_to_follow, error_id);
                // ESP_LOGI(SERIAL_TAG, "PACKET SENT OUT, remaining to send: %d, past sent: %d", num_to_follow, num_sent);
                num_sent++;
            }
            
            
        }
    }

    vTaskDelete(NULL);
}


/**
 * @brief A FreeRTOS task that checks the serial queue (given by Espressif's UART API)
 * and reacts according to the command given by the queue event. If the event indicates
 * valid data, send the data to the serial input handler, which parses the lines for
 * packets.
 */
void serial_read_task(void *params)
{

    uart_event_t event; /* Initialize a copy of an event object to receive Queue data */
    int receiving_packet = 0;   /* Keeps track of whether there are parts of packets waiting to be parsed in the input handler */

    for (;;)
    {
        if (xQueueReceive(uart_handle, (void *)&event, (portTickType)portMAX_DELAY))
        {
            switch (event.type)
            {

            case UART_DATA:;        /* Means there is regular data on the UART line */
                char *data = (char *)malloc(sizeof(char) * event.size + 1);
                data[event.size] = 0;   /* Append 0 to end of data for certainty */
                uart_read_bytes(UART_INSTANCE, data, event.size, 2);
                // ESP_LOGI(SERIAL_TAG, "DATA HERE! Size: %d", event.size);
                receiving_packet = handle_serial_input(data, event.size, receiving_packet);
                free(data);
                data = NULL;
                break;

            case UART_BUFFER_FULL:  /* The buffer has overflowed. Packets corrupted */
                ESP_LOGI(SERIAL_TAG, "[BUFFER_FULL]:");
                break;

            default:;
                // ESP_LOGI(SERIAL_TAG, "[NOT HANDLED]: %d", event.type);
            }
        }
    }

    vTaskDelete(NULL);
}


/**
 * @brief A FreeRTOS task that accepts a uart_write_evt_t struct from the queue,
 * and writes to the UART in the fashion indicated by the struct command variable.
 * It can send packets with embedded data, send an ESP_LOG message, or write a
 * stop message.
 */
void serial_write_task(void *params)
{
    uart_write_evt_t event; /* Placeholder for queue to copy into for each event */

    for (;;)
    {
        if (xQueueReceive(uart_write_handle, (void *)&event, (portTickType)portMAX_DELAY))
        {
            switch (event.cmd)
            {
            /* uart_write_bytes copies data to TX ring bufffer,
            possibly blocking until there is space */
            case SEND_PACKET: ;
                if (event.size != 256) break;
                char type_data[3] = "U1";
                uart_write_bytes(UART_INSTANCE, PACKET_HEADER, 5);
                uart_write_bytes(UART_INSTANCE, type_data, 3);
                uart_write_bytes(UART_INSTANCE, (void *)event.data, event.size);
                uint16_t pos_data[2] = {event.num_to_follow, event.error_id};
                uart_write_bytes(UART_INSTANCE, (void *)pos_data, 4);
                uart_write_bytes(UART_INSTANCE, PACKET_FOOTER, 4);
                break;
            case SEND_MSG:
                ESP_LOGI(SERIAL_TAG, "%s", event.data);
                break;
            case SEND_STOP:
                uart_write_bytes(UART_INSTANCE, STOP_MSG, STOP_MSG_LENGTH);
                break;
            case SEND_GENERIC:
                uart_write_bytes(UART_INSTANCE, event.data, event.size);
                break;
            case SEND_TEST:
                ESP_LOGI(SERIAL_TAG, "TEST");
            default:
                break;
            }

            if (event.data != NULL)
                free(event.data);
        }
    }
}


void app_main(void)
{
    init_uart_0(115200);
    // algo_white_init(128);
    // fad_algo = algo_white;
    algo_white_init(128);
    fad_algo = algo_white;

    /* Create queues for Uart Write task and Packet Handler task */
    uart_write_handle = xQueueCreate(10, sizeof(uart_write_evt_t));
    packet_queue = xQueueCreate(5, sizeof(packet));

    TaskHandle_t uart_read_task, uart_write_task, packet_task; // Throwaway vars, but needed

    /* Create and begin tasks located in functions given by first argument */
    xTaskCreate(serial_read_task, "Uart0 Queue Task", 2048, 
                NULL, configMAX_PRIORITIES - 3, &uart_read_task);
    xTaskCreate(serial_write_task, "Uart0 Write Task", 2048, 
                NULL, configMAX_PRIORITIES - 3, &uart_write_task);
    xTaskCreate(packet_handler_task, "Packet Handler", 2048, 
                NULL, configMAX_PRIORITIES - 4, &packet_task);

}
