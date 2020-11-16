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

#include "algo_white.h"
#include "esp_system.h"

void algo_white(uint16_t *adc_buff, uint8_t *dac_buff, uint16_t adc_pos, uint16_t dac_pos, int multisamples) {
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
	dac_buff[d_pos] = 0x7F;

	//next_avg and cur_avg hold current and next set of multisampled ADC values, averaged together. Should be 12 bit unsigned
	uint16_t next_avg = ((adc_buff[adc_pos] + adc_buff[adc_pos + 1]) >> 1); //bit shift 1 to limit output to 12 bit
	uint16_t cur_avg;

	for(int i = 0; i < (algo_white_size / multisamples) - 1; i++) {

		cur_avg = next_avg;
		
		/** TODO: FIX AVERAGING SCHEME TO AVERAGE MULTISAMPLES FOR MORE THAN 2 **/
		next_avg = (adc_buff[adc_pos + (i * 2) + 2] + adc_buff[adc_pos + (i * 2) + 3]) >> 1;

		// //to center input wave around 0 plus some, we effectively take discrete derivative, then discrete integral

		int8_t discrete_diff = (int8_t) ((next_avg - cur_avg) >> 4);

		// uint8_t integral_sum = dac_buff[dac_pos + i] + discrete_diff;

		
		// dac_buff[dac_pos + i + 1] = integral_sum;

		// if( dac_buff[dac_pos + i + 1] > (diff) )
		//  	dac_buff[dac_pos + i + 1] = (diff / 2);
		dac_buff[dac_pos + i + 1] = 0x7F + (((char)esp_random() * discrete_diff) >> 8);
 
		//bitwise AND ADC_buff value (12-bit to 8-bit) by some random char. Should make some input-dependent noise.
		// dac_buff[d_pos] = output;
	}
	// ESP_LOGI(ALGO_TAG, "Algorithm Running, diff: %4d, output: %3d", diff, dac_buff[5]);
}


void algo_white_init(int algo_size) {
	algo_white_size = algo_size;
}