/**
 * algo_test.c
 * Author: Corey Bean,
 * Organization: Messiah Collaboratory
 * Date: 9/23/2020
 *
 * Description:
 * This file runs through a test algorithm for our masker. It always outputs a signal, but varies its
 * volume depending on the magnitude of the ADC input average.
 *
 */

#include "fad_defs.h"
#include "esp_log.h"

//Usable portion of buffer to perform the algorithm, as other portions fill up
#define ALGO_TEST_SIZE ADC_BUFFER_SIZE
#define BIT_SHIFT 10 //To calculate bit shift, use ALGO_TEST_SIZE * max ADC input to find range of running_ADC_average; 
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
    uint8_t val = running_ADC_average >> 24; //use only MSB of running_ADC_average
	ESP_LOGI(ALGO_TAG, "Algorithm Running, average: %ud, val: %ud" , running_ADC_average, val);
    running_ADC_average = 0x3FFFFFFF;

	for(int i = 0;  i < (adc_algo_size / MULTISAMPLES); i++) {
		
		dac_buff[d_pos++] = (i % 2) ? val : 0;

        running_ADC_average += (adc_buff[a_pos++] << BIT_SHIFT);
        running_ADC_average += (adc_buff[a_pos++] << BIT_SHIFT);
	}
}

void algo_test_init() {
	ALGO_TAG = "ALGO_TEST";
	adc_algo_size = ALGO_TEST_SIZE;
	algo_function = algo_test; //from fad_defs.h
    running_ADC_average = 0x3FFFFFFF; //start less than halfay to max value
    }
