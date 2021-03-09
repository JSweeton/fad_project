#include <stdint.h>
#include "esp_bt_defs.h"
#include "fad_defs.h"

/**
 * event enumerations for fad_hdl_stack_evt
 */
enum {
	FAD_APP_EVT_STACK_UP,
	FAD_TEST_EVT,
	FAD_BT_ADDR_FOUND,
	FAD_BT_DEVICE_CONN,
	ADC_INIT_EVT,
	FAD_BT_NEED_GAP,
	FAD_BT_SETTING_CHANGED,
	FAD_DAC_BUFFER_READY,
	FAD_ADC_BUFFER_READY,
	FAD_ALGO_CHANGED,
} stack_evt;

/** 
 * Param definitions for stack events
 */
typedef union {

	/* for FAD_BT_ADDR_FOUND event */
	struct addr_found_param_t {
		esp_bd_addr_t peer_addr;	/* The address of the connectable device */
		bool from_nvs;				/* True if the address came from storage, false if GAP */
	} addr_found;

	/* FAD_DAC_BUFFER_READY */
	struct dac_buff_ready_param_t {
		uint8_t *buffer;	/* The pointer to the output buffer */
		int len;			/* The number of values ready for output */
	} dac_buff_ready;


	/* FAD_ADC_BUFFER_READY */
	struct adc_buffer_rdy_param {
		uint16_t adc_pos;
		uint16_t dac_pos;
	} adc_buff_pos_info;

	/* FAD_ALGO_CHANGE */
	struct adc_change_algo_param_t {
		algo_type_t algo_type;
		char * algo_name;
		algo_func_t algo_func;
		algo_init_func_t algo_init;
		algo_deinit_func_t algo_deinit;
	} change_algo;

} fad_main_cb_param_t;

/**
 * @brief Event handler for general tasks. Includes initializing ADC and event queues/tasks
 * @param evt A stack_evt enum
 * @param params Any parameters for the event
 */
void fad_hdl_stack_evt(uint16_t evt, void *params);