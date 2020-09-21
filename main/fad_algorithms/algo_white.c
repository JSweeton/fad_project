/**
 * algo_white.c
 * Author: Corey Bean,
 * Organization: Messiah Collaboratory
 * Date: 9/17/2020
 *
 * Description:
 * This file runs through the white noise algorithm for our masker.
 */

#include "fad_defs.h"
#include "esp_log.h"

//Usable portion of buffer to perform the algorithm, as other portions fill up
#define ALGO_WHITE_SIZE (ADC_BUFFER_SIZE / 2)

/**
 * @brief White noise algorithm for ESP masker
 * @param adc_buff Buffer that points to the beginning of the ADC data
 * @param dac_buff [OUT] Buffer that points to the beggining of DAC data staged to be output to the DAC
 */
void algo_white(uint16_t *adc_buff, uint8_t *dac_buff, uint16_t adc_pos, uint16_t dac_pos) {

	for(int i = 0; i < adc_algo_size; i++) {
		uint16_t pos = dac_pos + i;
		if( pos > DAC_BUFFER_SIZE ) {
			ESP_LOGW(ALGO_TAG, "Writing to invalid memory, DAC_POS: %d", dac_pos);
		} else {
			dac_buff[pos] = (i % 2) ? 255 : 0; // set sawtooth wave (when i is even, set 255 : odd, 0)
		}
	}
	ESP_LOGD(ALGO_TAG, "Algorithm Running, dac_pos: %d", dac_pos);
}

void algo_white_init(algo_func_t *algo_function) {
	ALGO_TAG = "ALGO_WHITE";
	adc_algo_size = ALGO_WHITE_SIZE;
	*algo_function = algo_white;
}
