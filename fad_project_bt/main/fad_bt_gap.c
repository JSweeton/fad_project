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

#include "esp_system.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_gap_bt_api.h"
#include "esp_log.h"

#include "main.h"
#include "fad_app_core.h"

static const char *GAP_TAG = "FAD_GAP";

static void fad_gap_cb(esp_bt_gap_cb_event_t evt, esp_bt_gap_cb_param_t *params);

esp_err_t fad_gap_start_discovery()
{
    esp_err_t err = 0;

    err = esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);

    err = esp_bt_gap_register_callback(fad_gap_cb);

    err = esp_bt_gap_start_discovery(ESP_BT_INQ_MODE_GENERAL_INQUIRY, 0x10, 0);

    return err;
}

static void found_address(esp_bd_addr_t peer_addr)
{
    fad_app_work_dispatch(fad_hdl_stack_evt, FAD_BT_ADDR_FOUND, NULL, 0, NULL);
}

static void fad_gap_cb(esp_bt_gap_cb_event_t evt, esp_bt_gap_cb_param_t *params)
{
    switch (evt)
    {
    case ESP_BT_GAP_DISC_STATE_CHANGED_EVT:
        if (params->disc_st_chg.state == ESP_BT_GAP_DISCOVERY_STARTED)
        {
            ESP_LOGI(GAP_TAG, "Discovery Started.");
        }
        else if (params->disc_st_chg.state == ESP_BT_GAP_DISCOVERY_STOPPED)
        {
            ESP_LOGI(GAP_TAG, "Discovery Stopped.");
        };
        break;

    case ESP_BT_GAP_DISC_RES_EVT:
        params->disc_res;
        char *addr = params->disc_res.bda;
        ESP_LOGI(GAP_TAG, "Device Found.\n ADDR: %x:%x:%x:%x:%x:%x",
        addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);
    default:
        ESP_LOGI(GAP_TAG, "CB Event: 0x%x", evt);
    }
}
