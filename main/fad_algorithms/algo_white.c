/**
 * algo_white.c
 * Author: Corey Bean,
 * Organization: Messiah Collaboratory
 * Date: 9/17/2020
 *
 * Description:
 * This file runs through the white noise algorithm for our masker.
 * Bit-wise AND with the input ADC and a pseudo-random number from esp_random 
 */

#include "fad_defs.h"
#include "esp_log.h"
#include "esp_system.h"

//Usable portion of buffer to perform the algorithm, as other portions fill up
#define ALGO_WHITE_SIZE (ADC_BUFFER_SIZE / 2)

/**
 * @brief White noise algorithm for ESP masker
 * @param adc_buff Buffer that points to the beginning of the ADC data
 * @param dac_buff [OUT] Buffer that points to the beggining of DAC data staged to be output to the DAC
 * @param adc_pos Points to starting point of this algorithm chunk
 * @param dac_pos Points to starting point of this algorithm chunk
 */
void algo_white(uint16_t *adc_buff, uint8_t *dac_buff, uint16_t adc_pos, uint16_t dac_pos) {
	uint16_t d_pos = dac_pos;
    uint16_t a_pos = adc_pos;

	//quick scan for max/min
	uint16_t max = 0;
	uint16_t min = 0xFFFF;
	for(int i = 0; i < 64; i++) {
		max = (max < adc_buff[a_pos + i]) ? adc_buff[a_pos + i] : max;
		min = (min > adc_buff[a_pos + i]) ? adc_buff[a_pos + i] : min;

	}
	uint16_t diff = (max - min) >> 4;
	dac_buff[d_pos] = diff >> 1;

	//next_avg and cur_avg hold current and next set of multisampled ADC values, averaged together. Should be 12 bit unsigned
	uint16_t next_avg = ((adc_buffer[adc_pos] + adc_buffer[adc_pos + 1]) >> 1); //bit shift 1 to limit output to 12 bit
	uint16_t cur_avg;

	for(int i = 0; i < (adc_algo_size / MULTISAMPLES) - 1; i++) {

		cur_avg = next_avg;
		
		/** TODO: FIX AVERAGING SCHEME TO AVERAGE MULTISAMPLES FOR MORE THAN 2 **/
		next_avg = (adc_buffer[adc_pos + (i * 2) + 2] + adc_buffer[adc_pos + (i * 2) + 3]) >> 1;

		//to center input wave around 0 plus some, we effectively take discrete derivative, then discrete integral

		int8_t discrete_diff = (int8_t) ((next_avg - cur_avg) >> 4);

		uint8_t integral_sum = dac_buff[dac_pos + i] + discrete_diff;

		
		dac_buff[dac_pos + i + 1] = integral_sum;

		if( dac_buff[dac_pos + i + 1] > (diff) )
		 	dac_buff[dac_pos + i + 1] = (diff / 2);
 
		//bitwise AND ADC_buff value (12-bit to 8-bit) by some random char. Should make some input-dependent noise.
		//dac_buff[i + 1] = ((unsigned char)(esp_random()) & dac_buff[i + 1]); 
		// dac_buff[d_pos] = output;
	}
	ESP_LOGI(ALGO_TAG, "Algorithm Running, diff: %4d, output: %3d", diff, dac_buff[5]);
}

/**
 * @brief Initialize white-noise algorithm
 * @param algo_function [out] pointer to algorithm function
 */
void algo_white_init() {
	ALGO_TAG = "ALGO_WHITE";
	adc_algo_size = ALGO_WHITE_SIZE;
	algo_function = algo_white; //from fad_defs.h
}
