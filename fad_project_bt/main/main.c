/**
 * main.c
 * Author: Corey Bean,
 * Organization: Messiah Collaboratory
 * Date: 9/17/2020
 *
 * Description:
 * This file initializes the fad_project program
 */

#include "main.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/uart.h"

#include "fad_app_core.h"
#include "fad_adc.h"
#include "fad_dac.h"
#include "fad_timer.h"
#include "fad_bt_main.h"
#include "fad_bt_gap.h"

#include "algo_template.h"

#define FAD_TAG "FAD"
#define FAD_NVS_NAMESPACE "FAD_BT" // Needed for NVS storage utilization

/* Determines whether program starts with test event. 0 for no test event, 1 for test event */
#define TEST_MODE 0

/* Potential states for state-machine behavior if main stack event handler gets too crowded */
// enum {
// 	FAD_INITIALIZING,
// 	FAD_WAITING,
// 	FAD_TESTING,
// 	FAD_INACTIVE_DAC_SETUP,
// 	FAD_INACTIVE_BT_SETUP,
// 	FAD_ACTIVE_BT_OUTPUT,
// 	FAD_ACTIVE_DAC_OUTPUT,
// 	FAD_ALGO_TRANSITION,
// } s_global_state;

static char s_nvs_addr_key[15] = "NVS_PEER_ADDR";
static esp_bd_addr_t s_peer_bda = {0, 0, 0, 0, 0, 0};

static algo_func_t s_algo_func = algo_template;
static int s_algo_read_size = 512; 

void app_main(void)
{
	/* create application task */
	fad_app_task_startup();

	if (TEST_MODE)
	{
		fad_app_work_dispatch(fad_hdl_stack_evt, FAD_TEST_EVT, NULL, 0, NULL);
	}
	else
	{
		if (fad_app_work_dispatch(fad_hdl_stack_evt, FAD_APP_EVT_STACK_UP, NULL, 0, NULL) != true)
		{
			ESP_LOGW(FAD_TAG, "Couldn't initiate stack");
		}
	}
}

void parse_error(esp_err_t err)
{
	if (err)
	{
		const char *err_string = esp_err_to_name(err);
		ESP_LOGE(FAD_TAG, "%s", err_string);
	}
}

void print_global_peer_addr()
{
	ESP_LOGI(FAD_TAG, "Peer Addr: %x:%x:%x:%x:%x:%x", s_peer_bda[0], s_peer_bda[1], s_peer_bda[2], s_peer_bda[3], s_peer_bda[4], s_peer_bda[5]);
}

/* Set stored address, given a peer address array */
void set_addr_in_nvs(esp_bd_addr_t peer_addr)
{
	nvs_handle_t nvs_handle = 0;
	uint64_t encoded_addr = 0;

	memcpy(&encoded_addr, peer_addr, 6);

	esp_err_t err;
	err = nvs_open(FAD_NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
	err = nvs_set_u64(nvs_handle, s_nvs_addr_key, encoded_addr);
	nvs_close(nvs_handle);

	if (err)
		ESP_LOGI(FAD_TAG, "Error detected");
	else
		ESP_LOGI(FAD_TAG, "Set successfully.");
	ESP_LOGI(FAD_TAG, "Set Addr: %x:%x:%x:%x:%x:%x", peer_addr[0], peer_addr[1], peer_addr[2], peer_addr[3], peer_addr[4], peer_addr[5]);
}

/* Get stored peer address from NVS and put it in global */
void get_addr_in_nvs()
{
	nvs_handle_t nvs_handle;
	uint64_t encoded_addr = 0;

	esp_err_t err;
	err = nvs_open(FAD_NVS_NAMESPACE, NVS_READONLY, &nvs_handle);
	err = nvs_get_u64(nvs_handle, s_nvs_addr_key, &encoded_addr);
	parse_error(err);
	nvs_close(nvs_handle);
	memcpy(s_peer_bda, &encoded_addr, 6);
}

void fad_hdl_stack_evt(uint16_t evt, void *params)
{
	fad_main_cb_param_t *p = (fad_main_cb_param_t *)params;
	esp_err_t err;
	switch (evt)
	{
	case (FAD_TEST_EVT):;

		ESP_LOGI(FAD_TAG, "Testing event...");
		print_global_peer_addr();
		ESP_LOGI(FAD_TAG, "Getting address in nvs...");
		get_addr_in_nvs();
		print_global_peer_addr();
		break;

	case (FAD_APP_EVT_STACK_UP):
		ESP_LOGI(FAD_TAG, "Initializing BT...");
		fad_bt_init();

		ESP_LOGI(FAD_TAG, "Starting NVS...");
		nvs_flash_init();

		ESP_LOGI(FAD_TAG, "Checking for stored device...");
		get_addr_in_nvs();

		// Check if there was a valid stored BDA address, skip gap if so
		if (s_peer_bda[0] != 0)
		{
			ESP_LOGI(FAD_TAG, "Stored device found, connecting...");

			fad_main_cb_param_t p;
			p.addr_found.from_nvs = true;

			fad_app_work_dispatch(fad_hdl_stack_evt, FAD_BT_ADDR_FOUND, (void *)&p, sizeof(fad_main_cb_param_t), NULL);
			break;
		}
		else
		{
			fad_app_work_dispatch(fad_hdl_stack_evt, FAD_BT_NEED_GAP, NULL, 0, NULL);
		}

		break;

	case FAD_BT_NEED_GAP: // No stored address, so initialize and begin gap search
		ESP_LOGI(FAD_TAG, "Starting GAP...");
		err = fad_gap_start_discovery();
		parse_error(err);
		break;

	case FAD_BT_ADDR_FOUND:; // BT ADDR found, try and connect. On failure, gap is attmepted again.

		if (!p->addr_found.from_nvs) // Check if address is not from NVS. Address from NVS is already stored and set in global
		{
			ESP_LOGI(FAD_TAG, "Storing address...");
			memcpy(s_peer_bda, p->addr_found.peer_addr, sizeof(esp_bd_addr_t));
			set_addr_in_nvs(s_peer_bda);
		}

		ESP_LOGI(FAD_TAG, "Found a bluetooth address %2x %2x %2x %2x %2x %2x...",
				 s_peer_bda[0], s_peer_bda[1], s_peer_bda[2], s_peer_bda[3], s_peer_bda[4], s_peer_bda[5]);

		ESP_LOGI(FAD_TAG, "Attempting to connect...");

		fad_bt_params param_to_bt;
		memcpy(param_to_bt.connect_params.peer_addr, s_peer_bda, sizeof(esp_bd_addr_t));
		fad_app_work_dispatch(fad_bt_stack_evt_handler, FAD_BT_CONNECT, (void *)&param_to_bt, sizeof(fad_bt_params), NULL);
		break;

	case FAD_BT_DEVICE_CONN: // Connection was successful. Prepare for transmission of data
		ESP_LOGI(FAD_TAG, "Connected to a target device via BT. Initiating physical connection...");
		/* Initialize physical audio measurements */
		err = adc_timer_init();
		err = adc_init();
		parse_error(err);

		/* TODO */
		break;

	case FAD_BT_SETTING_CHANGED:
		/* If a volume setting, or turns bluetooth off, etc. */
		ESP_LOGI(FAD_TAG, "User change of BT setting.");
		/* TODO */
		break;

	case FAD_ALGO_CHANGED: // The selected algorithm must be reselected.
		ESP_LOGI(FAD_TAG, "Changing algorithm to %s.", p->change_algo.algo_name);
		/* TODO */
		break;

	case FAD_ADC_BUFFER_READY:;
		struct adc_buffer_rdy_param buff = p->adc_buff_pos_info;
		s_algo_func(adc_buffer, dac_buffer, buff.adc_pos, buff.dac_pos, MULTISAMPLES);

	/* TAKING ADVANTAGE OF FALL THROUGH BEHAVIOR. ALGO_FUNC WILL PREPARE DAC BUFFER */
		__attribute__ ((fallthrough));	// Tell compiler to silence fallthrough warning
	case FAD_DAC_BUFFER_READY:;
		fad_bt_params buff_info = {
			.adv_buff_params.buffer = dac_buffer + buff.dac_pos,
			.adv_buff_params.num_vals = s_algo_read_size / MULTISAMPLES,
	};

		fad_app_work_dispatch(fad_bt_stack_evt_handler, FAD_BT_ADVANCE_BUFF,
							(void *)&buff_info, sizeof(fad_bt_params), NULL);
		break;

default:
	ESP_LOGI(FAD_TAG, "Unhandled event %d", evt);
	break;
}
}
