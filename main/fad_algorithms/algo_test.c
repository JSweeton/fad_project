/**
 * algo_test.c
 * Author: Corey Bean,
 * Organization: Messiah Collaboratory
 * Date: 9/23/2020
 *
 * Description:
 * This file runs through a test algorithm for our masker. It always outputs a signal, but varies its
 * volume depending on the magnitude of the ADC input average.
 */

#include "fad_defs.h"
#include "esp_log.h"

//Usable portion of buffer to perform the algorithm, as other portions fill up
#define ALGO_TEST_SIZE ADC_BUFFER_SIZE
#define BIT_SHIFT 9 //To calculate bit shift, use ALGO_TEST_SIZE * max ADC input to find range of running_ADC_average; 
                    //  need max value of ALGO_TEST_SIZE * adc_input to be 2 ^ 31

uint32_t running_ADC_average;

/**
 * @brief White noise algorithm for ESP masker
 * @param adc_buff Buffer that points to the beginning of the ADC data
 * @param dac_buff [OUT] Buffer that points to the beggining of DAC data staged to be output to the DAC
 */
void algo_test(uint16_t *adc_buff, uint8_t *dac_buff, uint16_t adc_pos, uint16_t dac_pos) {
    uint16_t d_pos = dac_pos;
    uint16_t a_pos = adc_pos;
    uint8_t val = 0;
	for(int i = 0;  i < adc_algo_size; i++) {
		
        val = running_ADC_average >> 24; //use only MSB of running_ADC_average

		dac_buff[d_pos] = val;

        running_ADC_average += (adc_buff[a_pos] << BIT_SHIFT);
        running_ADC_average -= (adc_buff[(a_pos + ALGO_TEST_SIZE - 1) % ADC_BUFFER_SIZE] << BIT_SHIFT);
        d_pos++; a_pos++;
        running_ADC_average += 1000;
	}
	ESP_LOGD(ALGO_TAG, "Algorithm Running, val: %d, average: %d", val, running_ADC_average);
}

void algo_test_init(algo_func_t *algo_function) {
	ALGO_TAG = "ALGO_TEST";
	adc_algo_size = ALGO_TEST_SIZE;
	*algo_function = algo_test;
    running_ADC_average = 0x7FFFFFFF; //start halfay to max value
    }
