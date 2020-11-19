#ifndef _FAD_ADC_H_
#define _FAD_ADC_H_


/**
 * Event enumeration for ADC-related events
 */
enum {
	ADC_BUFFER_READY_EVT,
} adc_evt;


typedef union {

	/**
	 * @brief ADC_BUFFER_READY_EVT
	 */
	struct adc_buffer_rdy_param {
		uint16_t adc_pos;
		uint16_t dac_pos;
	} buff_pos;

} adc_evt_params;

/**
 * Function Prototypes
 */

esp_err_t adc_init(void); //initializes ADC parameters
void adc_hdl_evt(uint16_t evt, void *params); //event handler for ADC events
int IRAM_ATTR local_adc1_read(int channel);

#endif