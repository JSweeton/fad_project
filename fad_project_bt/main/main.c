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

#include "algo_white.h"
#include "algo_test.h"

#define FAD_TAG "FAD"
#define FAD_NVS_NAMESPACE "FAD_BT"

/* Determines whether program starts with test event. 0 for no test event, 1 for test event */
#define TEST_MODE 0

static char s_nvs_addr_key[15] = "NVS_PEER_ADDR";
static esp_bd_addr_t s_peer_bda = {0, 0, 0, 0, 0, 0};

void app_main(void)
{
	// Initialize NVS.
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
	{
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}

	ESP_ERROR_CHECK(ret);

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
	if (err) {
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
	fad_gap_cb_param_t *p = (fad_gap_cb_param_t *)params;
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

			fad_gap_cb_param_t p;
			p.addr_found.from_nvs = true;

			fad_app_work_dispatch(fad_hdl_stack_evt, FAD_BT_ADDR_FOUND, (void*)&p, sizeof(fad_gap_cb_param_t), NULL);
			break;
		} else {
			fad_app_work_dispatch(fad_hdl_stack_evt, FAD_BT_NEED_GAP, NULL, 0, NULL);
		}

		break;

	case FAD_BT_NEED_GAP:
			ESP_LOGI(FAD_TAG, "Starting GAP...");
			esp_err_t err = fad_gap_start_discovery();
			parse_error(err);
		break;

	case FAD_BT_ADDR_FOUND:;

		if (!p->addr_found.from_nvs) // Check if address is not from NVS. Address from NVS is already stored and set in global
		{
			ESP_LOGI(FAD_TAG, "Storing address...");
			memcpy(s_peer_bda, p->addr_found.peer_addr, sizeof(esp_bd_addr_t));
			set_addr_in_nvs(s_peer_bda);
		}

		ESP_LOGI(FAD_TAG, "Found a bluetooth address %2x %2x %2x %2x %2x %2x...",
				 s_peer_bda[0], s_peer_bda[1], s_peer_bda[2], s_peer_bda[3], s_peer_bda[4], s_peer_bda[5]);

		ESP_LOGI(FAD_TAG, "Attempting to connect...");
		fad_bt_connect(s_peer_bda);
		break;

	case FAD_BT_DEVICE_CONN:
		ESP_LOGI(FAD_TAG, "Connected to a target device via BT. Initiating physical connection...");
		/* Initialize physical audio measurements */
		esp_err_t err = adc_timer_init();
		err = adc_init();
		parse_error(err);

		/* TODO */
		break;

	case FAD_BT_SETTING_CHANGED:
		ESP_LOGI(FAD_TAG, "User change of BT setting.");
		/* TODO */
		break;

	default:
		ESP_LOGI(FAD_TAG, "Unhandled event %d", evt);
		break;
	}
}
