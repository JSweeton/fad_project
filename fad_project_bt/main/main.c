/**
 * main.c
 * Author: Corey Bean,
 * Organization: Messiah Collaboratory
 * Date: 9/17/2020
 *
 * Description:
 * This file initializes the fad_project program and manages global events.
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
#include "fad_gpio.h"
#include "fad_timer.h"
#include "fad_bt_main.h"
#include "fad_bt_gap.h"
#include "fad_gpio.h"

#include "algo_template.h"
#include "algo_delay.h"


#define FAD_TAG "FAD"
#define FAD_NVS_NAMESPACE "FAD_BT" // Needed for NVS storage utilization

/* Determines whether program starts with test event. 0 for no test event, 1 for test event */
#define TEST_MODE 0

static char s_nvs_addr_key[15] = "NVS_PEER_ADDR";
static char s_nvs_algo_key[15] = "NVS_ALGO_INFO";
static esp_bd_addr_t s_peer_bda = {0, 0, 0, 0, 0, 0};

/* Algo function variables, subject to change on algorithm change. */
static algo_func_t s_algo_func = algo_template; //was algo_template
static int s_algo_read_size = 512;
static algo_deinit_func_t s_algo_deinit_func = algo_template_deinit;

/* Testing vars */
static int s_adc_calls = 0;

/* Called on ESP32 startup */
void app_main(void)
{
	/* create application task. Used to send events to event handlers */
	fad_app_task_startup();

	if (TEST_MODE)
	{
		fad_app_work_dispatch(fad_main_stack_evt_handler, FAD_TEST_EVT, NULL, 0, NULL);
	}
	else
	{
		if (fad_app_work_dispatch(fad_main_stack_evt_handler, FAD_APP_EVT_STACK_UP, NULL, 0, NULL) != true)
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

void set_algo_in_nvs(fad_algo_type_t type, fad_algo_mode_t mode)
{
	nvs_handle_t nvs_handle = 0;

	// place type in upper 32 bits, mode in lower 32 bits.
	uint64_t encoded_type_mode = ((uint64_t) type << 32) + (uint64_t) mode;

	esp_err_t err;
	err = nvs_open(FAD_NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
	err = nvs_set_u64(nvs_handle, s_nvs_algo_key, encoded_type_mode);
	nvs_close(nvs_handle);

	if (err)
		return parse_error(err);
	ESP_LOGI(FAD_TAG, "New algo set successfully.");

}

void get_algo_in_nvs(fad_algo_type_t *type, fad_algo_mode_t *mode) 
{
	nvs_handle_t nvs_handle;
	uint64_t encoded_type_mode = 0;

	esp_err_t err;
	err = nvs_open(FAD_NVS_NAMESPACE, NVS_READONLY, &nvs_handle);
	err = nvs_get_u64(nvs_handle, s_nvs_algo_key, &encoded_type_mode);
	parse_error(err);
	nvs_close(nvs_handle);
	*type = encoded_type_mode >> 32;
	*mode = (uint32_t) encoded_type_mode;
}

/* Set stored address, given a peer address array */
void set_addr_in_nvs(esp_bd_addr_t peer_addr)
{
	nvs_handle_t nvs_handle = 0;
	uint64_t encoded_addr = 0;

	memcpy(&encoded_addr, peer_addr, 6); // Simply copy the byte values from peer_addr into uint64

	esp_err_t err;
	err = nvs_open(FAD_NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
	err = nvs_set_u64(nvs_handle, s_nvs_addr_key, encoded_addr);
	nvs_close(nvs_handle);

	if (err)
		return parse_error(err);
	ESP_LOGI(FAD_TAG, "New addr set successfully.");
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

/* Parse new algorithm and initialize / setup based on new algo */
void handle_algo_change(fad_algo_type_t type, fad_algo_mode_t mode)
{
	s_algo_deinit_func();

	switch (type)
	{
	case FAD_ALGO_TEMPLATE:
		ESP_LOGI(FAD_TAG, "Changing algo to Template, Mode %d", mode);
		s_algo_func = algo_template;
		s_algo_deinit_func = algo_template_deinit;
		s_algo_read_size = 512;
		int template_period = 0;
		switch (mode)
		{
		case FAD_ALGO_MODE_1:
			template_period = 8;
			break;
		case FAD_ALGO_MODE_2:
			template_period = 16;
			break;
		case FAD_ALGO_MODE_3:
			template_period = 64;
			break;
		default:
			break;
		}
		fad_algo_init_params_t init_params = {
			.algo_template_params.read_size = s_algo_read_size,
			.algo_template_params.period = template_period
		};
		algo_template_init(&init_params);
		break;

	case FAD_ALGO_FREQ_SHIFT:
		/*
		ESP_LOGI(FAD_TAG, "Changing algo to Template, Mode %d", mode);
		s_algo_func = algo_freq_shift;
		s_algo_deinit_func = algo_template_deinit;
		s_algo_read_size = 512;
		int shift_amount = 2;
		
		algo_freq_shift_params_t init_params = {
			.algo_freq_shift_params.read_size = s_algo_read_size,
			.algo_freq_shift_params.shift_amount = shift_amount
		};
		
		//algo_freq_init(&init_params);
		//algo_freq_init(&init_params);
		break;
		*/

	case FAD_ALGO_PLL:
	case FAD_ALGO_DELAY:
		ESP_LOGI(FAD_TAG, "Changing algo to Template, Mode %d", mode);
		s_algo_func = algo_delay;
		s_algo_deinit_func = algo_delay_deinit;
		s_algo_read_size = 512;
	algo_delay_init();
	default:
		ESP_LOGI(FAD_TAG, "Unhandled algo function %d", type);
		break;
	}
}

void fad_main_stack_evt_handler(uint16_t evt, void *params)
{
	ESP_LOGD(FAD_TAG, "Main Stack evt: %d", evt);
	fad_main_cb_param_t *p = (fad_main_cb_param_t *)params;
	esp_err_t err;
	switch (evt)
	{
	case FAD_TEST_EVT:	
		err = adc_timer_init();
		err = adc_init();
		err = dac_init();
		err = adc_timer_set_read_size(s_algo_read_size);
		
		parse_error(err);
		adc_timer_start();
		break;

	case FAD_APP_EVT_STACK_UP:
		ESP_LOGI(FAD_TAG, "Initializing NVS...");
		nvs_flash_init();

		ESP_LOGI(FAD_TAG, "Loading stored algorithm...");
		fad_algo_mode_t mode = FAD_ALGO_MODE_1;
		fad_algo_type_t type = FAD_ALGO_DELAY;
		get_algo_in_nvs(&type, &mode);
		handle_algo_change(type, mode);

		fad_gpio_init();
		fad_gpio_begin_polling();

		
		ESP_LOGI(FAD_TAG, "Checking for stored device...");
		get_addr_in_nvs();

		bool wired_output_exists = true;

		if (wired_output_exists)
		{
			adc_timer_set_mode(FAD_OUTPUT_DAC);
			fad_app_work_dispatch(fad_main_stack_evt_handler, FAD_OUTPUT_READY, NULL, 0, NULL);
			break;
		}

		// Else, set up BT

		adc_timer_set_mode(FAD_OUTPUT_BT);
		fad_bt_init();

		// Check if there was a valid stored BDA address, skip gap if so
		if (s_peer_bda[0] != 0)
		{
			ESP_LOGI(FAD_TAG, "Stored device found, connecting...");

			fad_main_cb_param_t p;
			p.addr_found.from_nvs = true;

			fad_app_work_dispatch(fad_main_stack_evt_handler, FAD_BT_ADDR_FOUND, (void *)&p, sizeof(fad_main_cb_param_t), NULL);
			break;
		}
		else
		{
			ESP_LOGI(FAD_TAG, "No device found. Waiting for user to start discovery.");
			 fad_app_work_dispatch(fad_main_stack_evt_handler, FAD_DISC_START, NULL, 0, NULL);
		}

		break;

	case FAD_DISC_START: // No stored address, so initialize and begin gap search
		ESP_LOGI(FAD_TAG, "Starting GAP...");
		err = fad_gap_start_discovery();
		parse_error(err);
		break;

	case FAD_BT_ADDR_FOUND: // BT ADDR found, try and connect. On failure, gap is attmepted again.

		if (!p->addr_found.from_nvs) // Check if address is not from NVS.
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

	case FAD_OUTPUT_READY: // Connection was successful. Prepare for transmission of data
		ESP_LOGI(FAD_TAG, "Connected to a target device. Initializing output...");
		/* Initialize physical audio measurements */
		err = adc_timer_init();
		err = adc_init();
		err = dac_init();
		err = adc_timer_set_read_size(s_algo_read_size);
		parse_error(err);
		adc_timer_start();
		break;

	case FAD_OUTPUT_DISCONNECT: // Disconnected from output device, halt adc and timer, etc.
		adc_timer_stop();
		break;

	case FAD_VOL_CHANGE:;
		int vol_diff = p->vol_change_info.vol_change;
		ESP_LOGI(FAD_TAG, "Volume changed by %d", vol_diff);
		break;

	case FAD_ALGO_CHANGED: // The selected algorithm must be reselected. Given type and mode.
		// handle_algo_change(&(p->change_algo));
		handle_algo_change(p->change_algo.algo_type, p->change_algo.algo_mode);
		break;

	case FAD_ADC_BUFFER_READY:;
		struct adc_buffer_rdy_param buff = p->adc_buff_pos_info;
		s_algo_func(adc_buffer, dac_buffer, buff.adc_pos, buff.dac_pos, MULTISAMPLES);
		 //ESP_LOGI(FAD_TAG, "Dac buffer: %d", dac_buffer[100]);
		 //if(++s_adc_calls % 128 == 0);
		 	//ESP_LOGI(FAD_TAG, "ADC Calls: %d", s_adc_calls);
		 	//ESP_LOGI(FAD_TAG, "ADC BUFFER READY, dac_pos: %d, adc_pos: %d", buff.dac_pos, buff.adc_pos);

		/* TAKING ADVANTAGE OF FALL THROUGH BEHAVIOR. ALGO_FUNC WILL PREPARE DAC BUFFER */
		__attribute__((fallthrough)); // Tell compiler to silence fallthrough warning
	case FAD_DAC_BUFFER_READY:;
		break;

	default:
		ESP_LOGI(FAD_TAG, "Unhandled event %d", evt);
		break;
	}
}
