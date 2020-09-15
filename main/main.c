

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
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


//event enumerations for fad_hdl_stack_evt
enum {
	FAD_APP_EVT_STACK_UP,
	ADC_INIT_EVT,
	};

//events for fad_evt
enum {
	FAD_HEARTBEAT_EVT,
};

#define FAD_TAG "FAD"

void fad_hdl_stack_evt(uint16_t evt, void *params);

void fad_main_evt(uint16_t evt, void *params);


xTimerHandle xHeartbeatHandle;
void * xHeartbeatId;

static void vHeartbeatHandler(xTimerHandle xTimer) {
	if ( fad_app_work_dispatch(fad_main_evt, FAD_HEARTBEAT_EVT, NULL, 0, NULL) != true ) {
		ESP_LOGW(FAD_TAG, "Couldn't post heartbeat");
	}
}

static void heartbeat() {

	xHeartbeatHandle = xTimerCreate("heartbeat", 100, pdTRUE, (void *) 0, vHeartbeatHandler);

	if ( xTimerStart(xHeartbeatHandle, portMAX_DELAY) != pdPASS ) {
		ESP_LOGW(FAD_TAG, "Couldn't start timer");
	}
}

void app_main(void)
{
    // Initialize NVS.
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    ESP_ERROR_CHECK( ret );

    /* create application task */
    fad_app_task_startup();

    /* Bluetooth device name, connection mode and profile set up */
    if (fad_app_work_dispatch(fad_hdl_stack_evt, FAD_APP_EVT_STACK_UP, NULL, 0, NULL) != true) {
    	ESP_LOGW(FAD_TAG, "Couldn't initiate stack");
    }

    ret = adc_init();

    ESP_ERROR_CHECK( ret );

    heartbeat();

}

void fad_main_evt(uint16_t evt, void *params) {

	switch (evt) {
	case (FAD_HEARTBEAT_EVT): {
		ESP_LOGI(FAD_TAG, "Heartbeat evt");
	}
	}
}


void fad_hdl_stack_evt(uint16_t evt, void *params) {
	switch (evt) {
	case (FAD_APP_EVT_STACK_UP):
		ESP_LOGI(FAD_TAG, "Initializing program:");
		break;
	default:
		ESP_LOGI(FAD_TAG, "Unhandled event %d", evt);
		break;
	}

}




