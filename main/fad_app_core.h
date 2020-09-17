#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>




/**
 * @brief     callback function for app events
 */
typedef void (* fad_app_cb_t) (uint16_t event, void *param);

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
