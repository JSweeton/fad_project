/*
 * Authors: Corey Bean,
 * Date: 9/17/2020
 */

#include "fad_defs.h"

/*
 * This file will hold the task handler for the
 * algorithm that tursn the input ADC into a masking output.
 */

//Usable portion of buffer to perform the algorithm, as other portions fill up
#define ALGO_WHITE_SIZE (ADC_BUFFER_SIZE / 2)

/*
 * Parameters:
 * input: adc_buff -> buffer that points to the beginning of the ADC data
 * output: dac_buff -> buffer that points to the beggining of DAC data staged to be output to the DAC
 */
void algo_white(int *adc_buff, int *dac_buff) {

	for(int i = 0; i < adc_algo_size; i++) {
		dac_buff[i] = (i % 2) ? 255 : 0; // set sawtooth wave (when i is even, set 255 : odd, 0)
	}
}

void algo_white_init() {
	adc_algo_size = ALGO_WHITE_SIZE;
}
