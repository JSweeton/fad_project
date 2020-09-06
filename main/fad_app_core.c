#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "fad_app_core.h"
#include "esp_log.h"


xTaskHandle fadTaskHandle;
xQueueHandle fadQueueHandle;

#define QUEUE_LENGTH 5
#define QUEUE_SIZE sizeof(task_msg)
#define STACK_DEPTH 100

#define APP_TAG "FAD_APP_CORE"

bool fad_app_work_dispatch(fad_app_cb_t p_cb, uint16_t event, void *p_params, int param_len, fad_app_copy_cb_t p_copy_cb) {

	//add event to task for it to be handled
	task_msg msg;
	memset(&msg, 0, sizeof(task_msg));

	msg.cb = p_cb;
	msg.evt = event;

	if(param_len > 0 && p_params) {
		if((msg.params = malloc(param_len)) != NULL) {
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

static void fad_app_task_handler(void *params) {
	task_msg msg;

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

void fad_app_task_startup() {

	fadQueueHandle = xQueueCreate(QUEUE_LENGTH, QUEUE_SIZE);

	if( fadQueueHandle == NULL ) {
		ESP_LOGI(APP_TAG, "Queue could not be created");
		return;
	}

	//initialize stack using freertos
	xTaskCreate(fad_app_task_handler, "FAD_Task_Handler", STACK_DEPTH,
			0, configMAX_PRIORITIES - 3, &fadTaskHandle);

}

void fad_app_task_shutdown() {
	vTaskDelete(fadTaskHandle);
	vQueueDelete(fadQueueHandle);
}

