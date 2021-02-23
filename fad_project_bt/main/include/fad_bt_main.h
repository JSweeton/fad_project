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

/**
 * @brief Initializes controller modules and Bluedroid functions. Must be called before any
 * other bluetooth actions.
 * 
 */
void fad_bt_init();


/**
 * @brief If address is stored, return a saved bluetooth address from flash storage.
 * 
 * @returns Bluetooth address as a pointer to a char array. Must be Free'd (using free(pointer) function)
 */
uint8_t *fad_bt_get_stored_bd_addr();

#endif