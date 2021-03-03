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

#ifndef _FAD_BT_GAP_H_
#define _FAD_BT_GAP_H_

#include "esp_system.h"
#include "esp_bt_defs.h"
#include "esp_gap_bt_api.h"

/**
 * @brief Initialize GAP and start discovery for a device. Returns address through
 * event to main stack event handler.
 */
esp_err_t fad_gap_start_discovery();

void fad_gap_cb(esp_bt_gap_cb_event_t evt, esp_bt_gap_cb_param_t *params);


#endif