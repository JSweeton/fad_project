/**
 * main.c
 * Author: Corey Bean,
 * Organization: Messiah Collaboratory
 * Date: 9/17/2020
 *
 * Description:
 * This file initializes the fad_project program
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_system.h"
#include "esp_log.h"

#include "fad_app_core.h"
#include "fad_adc.h"
#include "fad_dac.h"
#include "fad_timer.h"

#include "fad_algorithms/algo_white.c"
#include "fad_algorithms/algo_test.c"


/**
 * event enumerations for fad_hdl_stack_evt
 */
enum {
	FAD_APP_EVT_STACK_UP,
	ADC_INIT_EVT,
} stack_evt;

/**
 * event enumerations for fad_hdl_main_evt
 */
enum {
	FAD_HEARTBEAT_EVT,
} main_evt;

#define FAD_TAG "FAD"

void fad_hdl_stack_evt(uint16_t evt, void *params);

void fad_hdl_main_evt(uint16_t evt, void *params);


// xTimerHandle xHeartbeatHandle;	//!	Handle for heartbeat timer
// void * xHeartbeatId;			//!	ID for heartbeat timer

// /**
//  * @brief Heartbeat timer handler called whenever timer goes off, adds event to the task queue
//  * @param xTimer
//  */
// static void vHeartbeatHandler(xTimerHandle xTimer) {

// 	if ( fad_app_work_dispatch(fad_hdl_main_evt, FAD_HEARTBEAT_EVT, NULL, 0, NULL) != true ) {
// 		ESP_LOGW(FAD_TAG, "Couldn't post heartbeat");
// 	}
// }

// /**
//  * @brief Initializes the heartbeat timer used for periodic tasks
//  */
// static void heartbeat() {
// 	xHeartbeatHandle = xTimerCreate("heartbeat", 100, pdTRUE, (void *) 0, vHeartbeatHandler);

// 	if ( xTimerStart(xHeartbeatHandle, portMAX_DELAY) != pdPASS ) {
// 		ESP_LOGW(FAD_TAG, "Couldn't start timer");
// 	}
// }

void app_main(void)
{
    // Initialize NVS.
    esp_err_t ret = nvs_flash_init();
    if ( ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND ) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    ESP_ERROR_CHECK( ret );

    /* create application task */
    fad_app_task_startup();
	
    if ( fad_app_work_dispatch(fad_hdl_stack_evt, FAD_APP_EVT_STACK_UP, NULL, 0, NULL) != true ) {
    	ESP_LOGW(FAD_TAG, "Couldn't initiate stack");
    }

}

/**
 * @brief Event handler for main_evt
 * @param evt A main_evt enum
 * @param params Any parameters, if they exist
 */
void fad_hdl_main_evt(uint16_t evt, void *params) {

	switch (evt) {
	default:
		ESP_LOGI(FAD_TAG, "Unknown Event");

	}
}


void stack_init(void) {
	esp_err_t ret;
	ret = adc_init(); 		//fad_adc.h
    ret = dac_init();		//fad_dac.h
	algo_white_init();		//fad_algorithms/algo_<func>.h
	ret = adc_timer_init();	//fad_timer.h

	if ( adc_timer_start() == ESP_OK ) {
		ESP_LOGI(FAD_TAG, "Started Timer");
	}

    ESP_ERROR_CHECK( ret );

	return;
}


/**
 * @brief Event handler for startup tasks. Includes initializing ADC and event queues/tasks
 * @param evt A stack_evt enum
 * @param params Any parameters for the event
 */
void fad_hdl_stack_evt(uint16_t evt, void *params) {
	switch (evt) {
	case (FAD_APP_EVT_STACK_UP):
		ESP_LOGI(FAD_TAG, "Initializing program:");
		stack_init();
		break;

	default:
		ESP_LOGI(FAD_TAG, "Unhandled event %d", evt);
		break;
	}

}




