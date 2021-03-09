/**
 * This file dictates GAP functionality of our device. GAP is Bluetooth's discovery
 * and connection protocol, and allows our device to search for and connect to peer devices
 * and discover provided services and capabilities.
 * 
 * We will potentially use GAP to find an appropriate audio device and store the devices'
 * bluetooth address for future connections.
 * 
 * Author: Corey Bean
 * Date: 2/18/2021
 * Organization: Collaboratory
 */

#ifndef _FAD_BT_MAIN_H_
#define _FAD_BT_MAIN_H_

#include "esp_system.h"
#include "esp_bt_defs.h"

enum  {
    FAD_BT_CONNECT,
    FAD_BT_ADVANCE_BUFF,
} fad_bt_events;

typedef union {
    /* FAD_BT_CONNECT */
    struct fad_bt_connect_params {
        esp_bd_addr_t peer_addr;
    } connect_params;

    /* FAD_BT_ADVANCE_BUFF */
    struct fad_bt_adv_buff_params {
        uint8_t *buffer;    // The address of the buffer ready to be read from
        int num_vals;       // The number of values that can be safely read from the buffer
    } adv_buff_params;

} fad_bt_params;

/**
 * 
 * @brief Initializes controller modules and Bluedroid functions. Must be called before any
 * other bluetooth actions.
 * 
 */
void fad_bt_init();

/**
 * @brief Begin A2DP and AVRCP connection to peer device
 * 
 * @param peer_addr The bluetooth address of the peer device
 */
void fad_bt_connect(esp_bd_addr_t peer_addr);

/**
 * @brief Func to send events for main bt events
 */
void fad_bt_stack_evt_handler(uint16_t event, void *param);

#endif