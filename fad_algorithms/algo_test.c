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

#include "algo_test.h"

void algo_test(uint16_t *adc_buff, uint8_t *dac_buff, uint16_t adc_pos, uint16_t dac_pos, int multisamples) {
    uint16_t d_pos = dac_pos;
    uint16_t a_pos = adc_pos;
    uint8_t val = running_ADC_average >> 24; //use only MSB of running_ADC_average
    running_ADC_average = 0x3FFFFFFF;

	for(int i = 0;  i < (algo_test_size / multisamples); i++) {
		
		dac_buff[d_pos++] = (i % 2) ? val : 0;

        running_ADC_average += (adc_buff[a_pos++] << BIT_SHIFT);
        running_ADC_average += (adc_buff[a_pos++] << BIT_SHIFT);
	}
}

void algo_test_init(int algo_size) {
    running_ADC_average = 0x3FFFFFFF; //start less than halfay to max value
    algo_test_size = algo_size;
}

