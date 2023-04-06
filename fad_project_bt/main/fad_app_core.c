/**
 * fad_app_core.c
 * Author: Corey Bean,
 * Organization: Messiah Collaboratory
 * Date: 9/17/2020
 *
 * Description:
 * This file starts the task handlers and queues for the FAD program.
 */

#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "fad_app_core.h"
#include "esp_log.h"
#include "fad_defs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"


xTaskHandle fadTaskHandle;
xQueueHandle fadQueueHandle;

#define QUEUE_LENGTH 5
#define QUEUE_SIZE sizeof(task_msg)
#define STACK_DEPTH 2048

#define APP_TAG "FAD_APP_CORE"

/**
 * @brief Given an event and any parameters and a destination function, places these items into the event queue
 * to be handled by their corresponding functions
 * @param p_cb Callback function that will receive the event
 * @param event Event enum to be passed to p_cb
 * @param p_params Pointer to the parameters for the event handling
 * @param param_len Length of p_param in bytes
 * @param p_copy_cb Deep copy function (not necessary)
 * @return
 * 		-True if success
 * 		-False if failure
 */
bool fad_app_work_dispatch(fad_app_cb_t p_cb, uint16_t event, void *p_params, int param_len, fad_app_copy_cb_t p_copy_cb) {

	//add event to task for it to be handled
	task_msg msg;
	memset(&msg, 0, sizeof(task_msg));

	msg.cb = p_cb;
	msg.evt = event;

	if(param_len > 0 && p_params) {
		if((msg.params = malloc(param_len)) != NULL) { //Checks if the parameters are empty
			memcpy(msg.params, p_params, param_len);
		} else {
			ESP_LOGW(APP_TAG, "No mem for param");
			return false;
		}
	}

	if(xQueueSend(fadQueueHandle, &msg, 10) != pdPASS) {
		ESP_LOGW(APP_TAG, "Queue full...");
		return false;
	} else {
		return true;
	}
}

/**
 * @brief FreeRTOS task to be running to receive tasks from queue and perform those tasks
 * @param params Startup params to be optionally passed through freeRTOS task creation
 */
static void fad_app_task_handler(void *params) {
	task_msg msg;
	ESP_LOGD(APP_TAG, "Beginning task...");

	for(;;) {
		BaseType_t isData = xQueueReceive(fadQueueHandle, &msg, 5);

		if(isData == pdPASS) {
			ESP_LOGD(APP_TAG, "%s event: 0x%x", __func__, msg.evt);
			msg.cb(msg.evt, msg.params);
			free(msg.params);
		}
	}
	vTaskDelete(fadTaskHandle);
}

/**
 * @brief Creates queue and app task for app functionality through the FreeRTOS API
 */
void fad_app_task_startup() {

	fadQueueHandle = xQueueCreate(QUEUE_LENGTH, QUEUE_SIZE);

	if( fadQueueHandle == NULL ) {
		ESP_LOGW(APP_TAG, "Queue could not be created");
		return;
	} else {
		ESP_LOGD(APP_TAG, "Queue Created");
	}

	//initialize stack using freertos
	if ( xTaskCreate(fad_app_task_handler, "FAD_Task_Handler", STACK_DEPTH,
					 0, configMAX_PRIORITIES - 4, &fadTaskHandle) != pdPASS) {
		ESP_LOGW(APP_TAG, "Could not create task");
	}

}

/**
 * @brief Shuts down task and event queue
 */

void fad_app_task_shutdown() {
	vTaskDelete(fadTaskHandle);
	vQueueDelete(fadQueueHandle);
}

