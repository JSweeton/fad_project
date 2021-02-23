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

static const char *BT_TAG = "FAD_BT";

void fad_bt_init()
{
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_BLE));

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();

    if (esp_bt_controller_init(&bt_cfg) != ESP_OK)
    {
        ESP_LOGE(BT_TAG, "%s initialize controller failed\n", __func__);
        return;
    }

    if (esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT) != ESP_OK)
    {
        ESP_LOGE(BT_TAG, "%s enable controller failed\n", __func__);
        return;
    }

    if (esp_bluedroid_init() != ESP_OK)
    {
        ESP_LOGE(BT_TAG, "%s initialize bluedroid failed\n", __func__);
        return;
    }

    if (esp_bluedroid_enable() != ESP_OK)
    {
        ESP_LOGE(BT_TAG, "%s enable bluedroid failed\n", __func__);
        return;
    }

}

/**
 * TODO
 */
uint8_t *fad_bt_get_stored_bd_addr()
{
    uint8_t *ret = (uint8_t *)malloc(sizeof(esp_bd_addr_t));
    esp_bd_addr_t temp[] = {{0,0,0,0,0,0}};
    memcpy(ret, temp, sizeof(esp_bd_addr_t));
    return ret; 
}

