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
 */
void algo_white(uint16_t *adc_buff, uint8_t *dac_buff, uint16_t adc_pos, uint16_t dac_pos) {
	uint16_t d_pos = dac_pos;
    uint16_t a_pos = adc_pos;
	//uint32_t rand = esp_random();
	for(int i = 0; i < adc_algo_size; i++) {

		//Attempted optimization, to only call esp_random once every 4 cycles. Not sure if it is any faster
		// int rand_pos = i % 4;
		// if(rand_pos = 0) rand = esp_random();
		// char output = (adc_buff[a_pos] >> 4) - (char) (rand >> (rand_pos * 8))

		//bitwise AND ADC_buff value (12-bit to 8-bit) by some random char. Should make some input-dependent noise.
		char output = (adc_buff[a_pos] >> 4) & ((unsigned char)(esp_random()) | 0x80); 
		dac_buff[d_pos] = output;
		d_pos++; a_pos++;
	}
	ESP_LOGD(ALGO_TAG, "Algorithm Running, dac_pos: %d", dac_pos);
}

/**
 * @brief Initialize white-noise algorithm
 * @param algo_function [out] pointer to algorithm function
 */
void algo_white_init(algo_func_t *algo_function) {
	ALGO_TAG = "ALGO_WHITE";
	adc_algo_size = ALGO_WHITE_SIZE;
	*algo_function = algo_white;
}
