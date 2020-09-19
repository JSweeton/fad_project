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

//Usable portion of buffer to perform the algorithm, as other portions fill up
#define ALGO_WHITE_SIZE (ADC_BUFFER_SIZE / 2)

/**
 * @brief White noise algorithm for ESP masker
 * @param adc_buff Buffer that points to the beginning of the ADC data
 * @param dac_buff [OUT] Buffer that points to the beggining of DAC data staged to be output to the DAC
 */
void algo_white(uint16_t *adc_buff, uint8_t *dac_buff) {

	for(int i = 0; i < adc_algo_size; i++) {
		dac_buff[i] = (i % 2) ? 255 : 0; // set sawtooth wave (when i is even, set 255 : odd, 0)
	}
}

void algo_white_init(algo_func_t *algo_function) {
	adc_algo_size = ALGO_WHITE_SIZE;
	*algo_function = algo_white;
}
