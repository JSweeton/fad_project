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

#include <string.h>
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
    /* Disable discoverability and connectability, end search */
    esp_bt_gap_cancel_discovery();
    esp_bt_gap_set_scan_mode(ESP_BT_NON_CONNECTABLE, ESP_BT_NON_DISCOVERABLE);

    /* Send found address back to main function for storage/connection */
    fad_gap_cb_param_t p;
    p.addr_found.from_nvs = false;
    memcpy(p.addr_found.peer_addr, peer_addr, sizeof(esp_bd_addr_t));

    fad_app_work_dispatch(fad_hdl_stack_evt, FAD_BT_ADDR_FOUND, (void*)&p, sizeof(fad_gap_cb_param_t), NULL);
    ESP_LOGI(GAP_TAG, "Set Addr: %x:%x:%x:%x:%x:%x", peer_addr[0], peer_addr[1], peer_addr[2], peer_addr[3], peer_addr[4], peer_addr[5]);
}

static bool get_name_from_eir(uint8_t *eir, uint8_t *bdname, uint8_t *bdname_len)
{
    uint8_t *rmt_bdname = NULL;
    uint8_t rmt_bdname_len = 0;

    if (!eir)
    {
        return false;
    }

    rmt_bdname = esp_bt_gap_resolve_eir_data(eir, ESP_BT_EIR_TYPE_CMPL_LOCAL_NAME, &rmt_bdname_len);
    if (!rmt_bdname)
    {
        rmt_bdname = esp_bt_gap_resolve_eir_data(eir, ESP_BT_EIR_TYPE_SHORT_LOCAL_NAME, &rmt_bdname_len);
    }

    if (rmt_bdname)
    {
        if (rmt_bdname_len > ESP_BT_GAP_MAX_BDNAME_LEN)
        {
            rmt_bdname_len = ESP_BT_GAP_MAX_BDNAME_LEN;
        }

        if (bdname)
        {
            memcpy(bdname, rmt_bdname, rmt_bdname_len);
            bdname[rmt_bdname_len] = '\0';
        }
        if (bdname_len)
        {
            *bdname_len = rmt_bdname_len;
        }
        return true;
    }

    return false;
}

static void parse_props(int num_props, esp_bt_gap_dev_prop_t *props, esp_bd_addr_t bda)
{
    if (!num_props)
        return;

    uint32_t cod = 0;
    int32_t rssi = -129; /* invalid value */
    uint8_t *eir = NULL;

    for (int i = 0; i < num_props; i++)
    {
        switch (props[i].type)
        {
        case ESP_BT_GAP_DEV_PROP_BDNAME:
            break;

        case ESP_BT_GAP_DEV_PROP_COD:;
            cod = *(uint32_t *)props[i].val;
            if (esp_bt_gap_is_valid_cod(cod))
            {
                uint32_t major = esp_bt_gap_get_cod_major_dev(cod);
                uint32_t srvc = esp_bt_gap_get_cod_srvc(cod);
                uint32_t minor = esp_bt_gap_get_cod_minor_dev(cod);
                ESP_LOGI(GAP_TAG, "Class of device: Major->%x Minor->%x Srvc->%x",
                     major, minor, srvc);
            }
            break;

        case ESP_BT_GAP_DEV_PROP_RSSI:
            rssi = *(int8_t *)props[i].val;
            break;

        case ESP_BT_GAP_DEV_PROP_EIR:
            eir = (uint8_t *)props[i].val;
            break;
        }
    }

    if (esp_bt_gap_is_valid_cod(cod))
    {
        uint32_t major = esp_bt_gap_get_cod_major_dev(cod);
        // uint32_t srvc = esp_bt_gap_get_cod_srvc(cod);
        uint32_t minor = esp_bt_gap_get_cod_minor_dev(cod);
        if (major != 4 || (minor != 6 && minor != 1))
        {
            ESP_LOGI(GAP_TAG, "Not correct COD. Major: %x, Minor: %x", major, minor);
            return;
        }
    }

    if (eir)
    {
        unsigned char *peer_bdname = (unsigned char *)malloc(ESP_BT_GAP_MAX_BDNAME_LEN);
        get_name_from_eir(eir, peer_bdname, NULL);

        ESP_LOGI(GAP_TAG, "Found a target device, name %s", peer_bdname);
    } else
    {
        ESP_LOGI(GAP_TAG, "Found a nameless target device.");
    }

    found_address(bda);
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

    case ESP_BT_GAP_DISC_RES_EVT:;
        unsigned char *addr = params->disc_res.bda;
        ESP_LOGI(GAP_TAG, "Device Found. ADDR: %x:%x:%x:%x:%x:%x",
                 addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);
        parse_props(params->disc_res.num_prop, params->disc_res.prop, params->disc_res.bda);
        break;
    default:
        ESP_LOGI(GAP_TAG, "CB Event: 0x%x", evt);
    }
}
