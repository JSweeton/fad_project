#ifndef _FAD_APP_CORE_H_
#define _FAD_APP_CORE_H_

#include <stdbool.h>
#include <stdio.h>
#include "fad_defs.h"
#include "freertos/FreeRTOS.h"


/* message for tasks */
typedef struct {
	uint16_t evt;
	fad_app_cb_t cb;
	void *params;
} task_msg;


typedef void (* fad_app_copy_cb_t) (task_msg *msg, void *p_dest, void *p_src);

/**
 * @brief Initialize the dispatching functions and tasks
 */
void fad_app_task_startup();

/**
 * @brief Add an event to the event queue to be sent out to the given callback function
 * 
 * @param p_cb The callback function for which the event will be sent to
 * @param event The enumerated event
 * @param p_params Extra parameters to be added that are associated with the event
 * @param param_len The length in bytes of p_params
 * @param p_copy_cb The deep copy callback. Not sure what this is for to be honest, can be NONE
 * @returns Boolean reprenting success of dispatch
 */
bool fad_app_work_dispatch(fad_app_cb_t p_cb, uint16_t event, void *p_params, int param_len, fad_app_copy_cb_t p_copy_cb);

/**
 * @brief Shut down the main dispatching task
 */
void fad_app_task_shutdown();

#endif