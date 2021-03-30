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
#include "esp_avrc_api.h"
#include "esp_a2dp_api.h"

#include "fad_bt_main.h"
#include "fad_app_core.h"
#include "fad_defs.h"
#include "main.h"

// AVRCP used transaction label
#define APP_RC_CT_TL_GET_CAPS (0)
#define APP_RC_CT_TL_RN_VOLUME_CHANGE (1)
#define APP_RC_CT_TL_RN_BATTERY_CHANGE (2)

// output aliasing (converts 1 output value to N output values)
#define FAD_OUTPUT_BT_ALIASING 4

enum
{
    APP_AV_MEDIA_STATE_IDLE,
    APP_AV_MEDIA_STATE_STARTING,
    APP_AV_MEDIA_STATE_STARTED,
    APP_AV_MEDIA_STATE_STOPPING,
};

enum
{
    A2DP_CONN_STATE_UNCONNECTED,
    A2DP_CONN_STATE_CONNECTING,
    A2DP_CONN_STATE_CONNECTED,
};

/// callback function for A2DP source
static void bt_app_a2d_cb(esp_a2d_cb_event_t event, esp_a2d_cb_param_t *param);

/// callback function for A2DP source audio data stream
static int32_t bt_app_a2d_data_cb(uint8_t *data, int32_t len);

/// callback function for AVRCP controller
static void bt_app_rc_ct_cb(esp_avrc_ct_cb_event_t event, esp_avrc_ct_cb_param_t *param);

static void bt_av_hdl_avrc_ct_evt(uint16_t event, void *p_param);

static void bt_secondary_stack_init();

static esp_avrc_rn_evt_cap_mask_t s_avrc_peer_rn_cap;
static const char *BT_TAG = "FAD_BT";
static int s_media_state = APP_AV_MEDIA_STATE_IDLE;
static int s_a2dp_conn_state = A2DP_CONN_STATE_UNCONNECTED;
static esp_bd_addr_t s_peer_bda = {0, 0, 0, 0, 0, 0};
static int s_out_pos = 0;
static int s_out_pos_limit;

/// Delay for when BT output is just beginning
static int s_buffer_fill_delay = 0;

static int s_num_calls = 0;

void fad_bt_stack_evt_handler(uint16_t event, void *param)
{
    ESP_LOGD(BT_TAG, "BT Stack evt: %d", event);
    fad_bt_params *p = param;
    switch (event)
    {
    case FAD_BT_CONNECT:
        memcpy(s_peer_bda, p->connect_params.peer_addr, sizeof(esp_bd_addr_t));
        bt_secondary_stack_init();
        esp_a2d_source_connect(s_peer_bda);
        s_a2dp_conn_state = A2DP_CONN_STATE_CONNECTING;
        break;

    case FAD_BT_ADVANCE_BUFF:
        // s_out_buffer = p->adv_buff_params.buffer;
        // s_out_pos_limit = p->adv_buff_params.num_vals;
        // s_out_pos = 0;
        break;
    }
}

static void bt_secondary_stack_init()
{
    esp_avrc_ct_init();
    esp_avrc_ct_register_callback(bt_app_rc_ct_cb);

    esp_avrc_rn_evt_cap_mask_t evt_set = {0};
    esp_avrc_rn_evt_bit_mask_operation(ESP_AVRC_BIT_MASK_OP_SET, &evt_set, ESP_AVRC_RN_VOLUME_CHANGE);
    // assert(esp_avrc_tg_set_rn_evt_cap(&evt_set) == ESP_OK);

    /* initialize A2DP source */
    esp_a2d_register_callback(bt_app_a2d_cb);
    esp_a2d_source_register_data_callback(bt_app_a2d_data_cb);
    esp_a2d_source_init();
}

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

static int32_t bt_app_a2d_data_cb(uint8_t *data, int32_t len)
{
    // DO NOT ESP_LOG IN HERE. Breaks bluetooth output. Only do it for debugging.

    if (len < 0 || data == NULL)
    {
        ESP_LOGI(BT_TAG, "Given length of 0");
        return 0;
    }

    /* Delay to let buffer fill to an appropriate level before data is read. Adds 256 sample delay (25ms). */
    if (s_buffer_fill_delay < 1)
    {
        for (int i = 0; i < len; i++)
        {
            data[i] = 0;
        }
        s_buffer_fill_delay++;
        return len;
    }

    /* 
    *  Output is formatted as follows: Left lower byte, left upper, right lower byte, right upper
    *  Our input is mono 8 bit, so right and left are equal with lower bytes set to 0
    *  Also, we utilize aliasing to transform our ~11 kHz output to 44.1 kHz
    */

    /* Sets how many outputs a single FAD output will expand to.
    4 for 8-bit mono to 16-bit stereo conversion, rest for aliasing */
    int scaler = FAD_OUTPUT_BT_ALIASING * 4;


    for (int i = 0; i < (len / scaler); i++) // for loop to go through each fad output value
    {
        for (int j = 0; j < FAD_OUTPUT_BT_ALIASING; j++)
        {
            // set lower bytes to 0
            data[(scaler * i) + (4 * j) + 0] = 0;
            data[(scaler * i) + (4 * j) + 2] = 0;

            // set left and right upper bytes to value
            data[(scaler * i) + (4 * j) + 1] = dac_buffer[s_out_pos];
            data[(scaler * i) + (4 * j) + 3] = dac_buffer[s_out_pos];
        }

        /* Advance and wrap buffer */
        s_out_pos++;
        if (s_out_pos >= DAC_BUFFER_SIZE)
            s_out_pos = 0;
    }

    return len;
}

static void bt_app_rc_ct_cb(esp_avrc_ct_cb_event_t event, esp_avrc_ct_cb_param_t *param)
{
    switch (event)
    {
    case ESP_AVRC_CT_METADATA_RSP_EVT:
    case ESP_AVRC_CT_CONNECTION_STATE_EVT:
    case ESP_AVRC_CT_PASSTHROUGH_RSP_EVT:
    case ESP_AVRC_CT_CHANGE_NOTIFY_EVT:
    case ESP_AVRC_CT_REMOTE_FEATURES_EVT:
    case ESP_AVRC_CT_GET_RN_CAPABILITIES_RSP_EVT:
    case ESP_AVRC_CT_SET_ABSOLUTE_VOLUME_RSP_EVT:
    {
        fad_app_work_dispatch(bt_av_hdl_avrc_ct_evt, event, param, sizeof(esp_avrc_ct_cb_param_t), NULL);
        break;
    }
    default:
        ESP_LOGE(BT_TAG, "Invalid AVRC event: %d", event);
        break;
    }
}

static void bt_av_volume_changed(void)
{
    if (esp_avrc_rn_evt_bit_mask_operation(ESP_AVRC_BIT_MASK_OP_TEST, &s_avrc_peer_rn_cap,
                                           ESP_AVRC_RN_VOLUME_CHANGE))
    {
        esp_avrc_ct_send_register_notification_cmd(APP_RC_CT_TL_RN_VOLUME_CHANGE, ESP_AVRC_RN_VOLUME_CHANGE, 0);
    }
}

static void bt_app_av_media_proc(uint16_t event, void *param)
{
    esp_a2d_cb_param_t *a2d = NULL;
    switch (s_media_state)
    {
    case APP_AV_MEDIA_STATE_IDLE:
    {
        /* Send Sink device a query to check if it is ready for media transmission */
        /* Parse response from check_src_ready query */
        if (event == ESP_A2D_MEDIA_CTRL_ACK_EVT)
        {
            a2d = (esp_a2d_cb_param_t *)(param);
            /* Send message to indicate starting of media transmission */
            if (a2d->media_ctrl_stat.cmd == ESP_A2D_MEDIA_CTRL_CHECK_SRC_RDY &&
                a2d->media_ctrl_stat.status == ESP_A2D_MEDIA_CTRL_ACK_SUCCESS)
            {
                ESP_LOGI(BT_TAG, "a2dp media ready, starting ...");
                esp_a2d_media_ctrl(ESP_A2D_MEDIA_CTRL_START);
                s_media_state = APP_AV_MEDIA_STATE_STARTING;
            }
        }
        break;
    }
    case APP_AV_MEDIA_STATE_STARTING:
    {
        if (event == ESP_A2D_MEDIA_CTRL_ACK_EVT)
        {
            a2d = (esp_a2d_cb_param_t *)(param);
            if (a2d->media_ctrl_stat.cmd == ESP_A2D_MEDIA_CTRL_START &&
                a2d->media_ctrl_stat.status == ESP_A2D_MEDIA_CTRL_ACK_SUCCESS)
            {
                ESP_LOGI(BT_TAG, "a2dp media start successfully.");
                s_media_state = APP_AV_MEDIA_STATE_STARTED;
            }
        }
        break;
    }
    case APP_AV_MEDIA_STATE_STARTED:
    {
    }
    case APP_AV_MEDIA_STATE_STOPPING:
    {
        if (event == ESP_A2D_MEDIA_CTRL_ACK_EVT)
        {
            a2d = (esp_a2d_cb_param_t *)(param);
            if (a2d->media_ctrl_stat.cmd == ESP_A2D_MEDIA_CTRL_STOP &&
                a2d->media_ctrl_stat.status == ESP_A2D_MEDIA_CTRL_ACK_SUCCESS)
            {
                ESP_LOGI(BT_TAG, "a2dp media stopped successfully, disconnecting...");
                s_media_state = APP_AV_MEDIA_STATE_IDLE;
                esp_a2d_source_disconnect(s_peer_bda);
            }
            else
            {
                ESP_LOGI(BT_TAG, "a2dp media stopping...");
                esp_a2d_media_ctrl(ESP_A2D_MEDIA_CTRL_STOP);
            }
        }
        break;
    }
    }
}

static void bt_app_a2d_cb(esp_a2d_cb_event_t event, esp_a2d_cb_param_t *param)
{
    esp_a2d_cb_param_t *a2d = NULL;
    switch (event)
    {
    case ESP_A2D_CONNECTION_STATE_EVT:
    {
        a2d = (esp_a2d_cb_param_t *)(param);
        if (a2d->conn_stat.state == ESP_A2D_CONNECTION_STATE_CONNECTED)
        {
            ESP_LOGI(BT_TAG, "a2dp connected");
            fad_app_work_dispatch(fad_main_stack_evt_handler, FAD_OUTPUT_READY, NULL, 0, NULL);
            ESP_LOGI(BT_TAG, "a2dp media ready checking ...");
            esp_a2d_media_ctrl(ESP_A2D_MEDIA_CTRL_CHECK_SRC_RDY);
            s_a2dp_conn_state = A2DP_CONN_STATE_CONNECTED;
        }
        else if (a2d->conn_stat.state == ESP_A2D_CONNECTION_STATE_DISCONNECTED)
        {
            ESP_LOGI(BT_TAG, "a2dp disconnected");
            if (s_a2dp_conn_state == A2DP_CONN_STATE_CONNECTING)
            {
                ESP_LOGI(BT_TAG, "Initializing GAP to find new device...");
                fad_app_work_dispatch(fad_main_stack_evt_handler, FAD_BT_NEED_GAP, NULL, 0, NULL);
            }
            else if (s_a2dp_conn_state == A2DP_CONN_STATE_CONNECTED)
            {
                fad_app_work_dispatch(fad_main_stack_evt_handler, FAD_OUTPUT_DISCONNECT, NULL, 0, NULL);
            }

            s_a2dp_conn_state = A2DP_CONN_STATE_UNCONNECTED;
        }
        break;
    }
    case ESP_A2D_AUDIO_STATE_EVT:
        a2d = (esp_a2d_cb_param_t *)(param);
        if (ESP_A2D_AUDIO_STATE_STARTED == a2d->audio_stat.state)
        {
            ESP_LOGI(BT_TAG, "Audio state started.");
        }
        break;
    case ESP_A2D_AUDIO_CFG_EVT:
    case ESP_A2D_MEDIA_CTRL_ACK_EVT:
        break;
    default:
        ESP_LOGE(BT_TAG, "%s unhandled evt %d", __func__, event);
        break;
    }
    bt_app_av_media_proc(event, param);
}

void bt_av_notify_evt_handler(uint8_t event_id, esp_avrc_rn_param_t *event_parameter)
{
    switch (event_id)
    {
    case ESP_AVRC_RN_VOLUME_CHANGE:
        ESP_LOGI(BT_TAG, "Volume changed: %d", event_parameter->volume);
        ESP_LOGI(BT_TAG, "Set absolute volume: volume %d", event_parameter->volume);
        esp_avrc_ct_send_set_absolute_volume_cmd(APP_RC_CT_TL_RN_VOLUME_CHANGE, event_parameter->volume);
        bt_av_volume_changed();
        break;
    case ESP_AVRC_RN_BATTERY_STATUS_CHANGE:
        /* options for value (event_parameter->batt) */
        // ESP_AVRC_BATT_NORMAL       = 0,               < normal state
        // ESP_AVRC_BATT_WARNING      = 1,               < unable to operate soon
        // ESP_AVRC_BATT_CRITICAL     = 2,               < cannot operate any more
        // ESP_AVRC_BATT_EXTERNAL     = 3,               < plugged to external power supply
        // ESP_AVRC_BATT_FULL_CHARGE  = 4,               < when completely charged from external power supply
        ESP_LOGI(BT_TAG, "Battery changed: %d", event_parameter->batt);
        if (esp_avrc_rn_evt_bit_mask_operation(ESP_AVRC_BIT_MASK_OP_TEST, &s_avrc_peer_rn_cap,
                                               ESP_AVRC_RN_BATTERY_STATUS_CHANGE))
        {
            esp_avrc_ct_send_register_notification_cmd(APP_RC_CT_TL_RN_BATTERY_CHANGE, ESP_AVRC_RN_BATTERY_STATUS_CHANGE, 0);
        }
    }
}

static void bt_av_hdl_avrc_ct_evt(uint16_t event, void *p_param)
{
    ESP_LOGI(BT_TAG, "%s evt %d", __func__, event);
    esp_avrc_ct_cb_param_t *rc = (esp_avrc_ct_cb_param_t *)(p_param);
    switch (event)
    {
    case ESP_AVRC_CT_CONNECTION_STATE_EVT:
    {
        uint8_t *bda = rc->conn_stat.remote_bda;
        ESP_LOGI(BT_TAG, "AVRC conn_state evt: state %d, [%02x:%02x:%02x:%02x:%02x:%02x]",
                 rc->conn_stat.connected, bda[0], bda[1], bda[2], bda[3], bda[4], bda[5]);

        if (rc->conn_stat.connected)
        {
            // get remote supported event_ids of peer AVRCP Target
            esp_avrc_ct_send_get_rn_capabilities_cmd(APP_RC_CT_TL_GET_CAPS);
        }
        else
        {
            // clear peer notification capability record
            s_avrc_peer_rn_cap.bits = 0;
        }
        break;
    }
    case ESP_AVRC_CT_PASSTHROUGH_RSP_EVT:
    {
        ESP_LOGI(BT_TAG, "AVRC passthrough rsp: key_code 0x%x, key_state %d", rc->psth_rsp.key_code, rc->psth_rsp.key_state);
        break;
    }
    case ESP_AVRC_CT_METADATA_RSP_EVT:
    {
        ESP_LOGI(BT_TAG, "AVRC metadata rsp: attribute id 0x%x, %s", rc->meta_rsp.attr_id, rc->meta_rsp.attr_text);
        free(rc->meta_rsp.attr_text);
        break;
    }
    case ESP_AVRC_CT_CHANGE_NOTIFY_EVT:
    {
        ESP_LOGI(BT_TAG, "AVRC event notification: %d", rc->change_ntf.event_id);
        bt_av_notify_evt_handler(rc->change_ntf.event_id, &rc->change_ntf.event_parameter);
        break;
    }
    case ESP_AVRC_CT_REMOTE_FEATURES_EVT:
    {
        ESP_LOGI(BT_TAG, "AVRC remote features %x, TG features %x", rc->rmt_feats.feat_mask, rc->rmt_feats.tg_feat_flag);
        break;
    }
    case ESP_AVRC_CT_GET_RN_CAPABILITIES_RSP_EVT:
    {
        ESP_LOGI(BT_TAG, "remote rn_cap: count %d, bitmask 0x%x", rc->get_rn_caps_rsp.cap_count,
                 rc->get_rn_caps_rsp.evt_set.bits);
        s_avrc_peer_rn_cap.bits = rc->get_rn_caps_rsp.evt_set.bits;

        /* Register for target (headphones) to send notifications to this device when volume status changes. Must still send set volume command to apply volume change. */
        bt_av_volume_changed();

        /* Register for target (headphones) to send notifications to this device when battery status changes. */
        if (esp_avrc_rn_evt_bit_mask_operation(ESP_AVRC_BIT_MASK_OP_TEST, &s_avrc_peer_rn_cap,
                                               ESP_AVRC_RN_BATTERY_STATUS_CHANGE))
        {
            ESP_LOGI(BT_TAG, "Has battery status change operation");
            esp_avrc_ct_send_register_notification_cmd(APP_RC_CT_TL_RN_VOLUME_CHANGE, ESP_AVRC_RN_BATTERY_STATUS_CHANGE, 0);
        }
        else
        {
            ESP_LOGI(BT_TAG, "No battery status nofification ability.");
        }

        break;
    }
    case ESP_AVRC_CT_SET_ABSOLUTE_VOLUME_RSP_EVT:
    {
        ESP_LOGI(BT_TAG, "Set absolute volume rsp: volume %d", rc->set_volume_rsp.volume);
        break;
    }

    default:
        ESP_LOGE(BT_TAG, "%s unhandled evt %d", __func__, event);
        break;
    }
}