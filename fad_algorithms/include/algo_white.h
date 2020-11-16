/**
 * algo_white.h
 * Author: Corey Bean,
 * Organization: Messiah Collaboratory
 * Date: 11/13/2020
 *
 * Description:
 * This file runs through the white noise algorithm for our masker.
 * Bit-wise AND with the input ADC and a pseudo-random number from esp_random 
 */

#include <stdint.h>


//Usable portion of buffer to perform the algorithm, as other portions fill up
int algo_white_size;

/**
 * @brief White noise algorithm for ESP masker
 * @param adc_buff Buffer that points to the beginning of the ADC data
 * @param dac_buff [OUT] Buffer that points to the beggining of DAC data staged to be output to the DAC
 * @param adc_pos Points to starting point of this algorithm chunk
 * @param dac_pos Points to starting point of this algorithm chunk
 * @param multisamples The number of input data per output
 */
void algo_white(uint16_t *adc_buff, uint8_t *dac_buff, uint16_t adc_pos, uint16_t dac_pos, int multisamples); 

/**
 * @brief Initialize white-noise algorithm
 * @param algo_function [out] pointer to algorithm function
 */
void algo_white_init(int algo_size); 
