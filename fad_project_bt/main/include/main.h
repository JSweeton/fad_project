#include <stdint.h>
#include "esp_bt_defs.h"

/**
 * event enumerations for fad_hdl_stack_evt
 */
enum {
	FAD_APP_EVT_STACK_UP,
	FAD_TEST_EVT,
	FAD_BT_ADDR_FOUND,
	FAD_BT_DEVICE_CONN,
	ADC_INIT_EVT,
} stack_evt;

/** 
 * Param definitions for stack events
 */
typedef union {

	/* FAD_BT_ADDR_FOUND */
	struct addr_found_param_t {
		esp_bd_addr_t peer_addr;	/* The address of the connectable device */
		bool from_svh;				/* Whether the address came from storage or GAP */
	} addr_found;

} fad_gap_cb_param_t;

/**
 * @brief Event handler for general tasks. Includes initializing ADC and event queues/tasks
 * @param evt A stack_evt enum
 * @param params Any parameters for the event
 */
void fad_hdl_stack_evt(uint16_t evt, void *params);