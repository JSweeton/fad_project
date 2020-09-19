#ifndef _FAD_APP_CORE_H_
#define _FAD_APP_CORE_H_

#include <stdbool.h>
#include <stdio.h>
#include "fad_defs.h"


//message for tasks
typedef struct {
	uint16_t evt;
	fad_app_cb_t cb;
	void *params;
} task_msg;


typedef void (* fad_app_copy_cb_t) (task_msg *msg, void *p_dest, void *p_src);

void fad_app_task_startup();

bool fad_app_work_dispatch(fad_app_cb_t p_cb, uint16_t event, void *p_params, int param_len, fad_app_copy_cb_t p_copy_cb);

void fad_app_task_shutdown();

#endif