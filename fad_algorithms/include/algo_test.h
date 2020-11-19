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

#include <stdint.h>

/* Usable portion of buffer to perform the algorithm, as other portions fill up. Should always be at least half of buffer size. */
//To calculate bit shift, use ALGO_TEST_SIZE * max ADC input to find range of running_ADC_average; 
#define BIT_SHIFT 10

uint32_t running_ADC_average;
uint16_t algo_test_size;

/**
 * @brief White noise algorithm for ESP masker
 * @param in_buff Buffer that points to the beginning of the ADC data
 * @param out_buff [OUT] Buffer that points to the beggining of DAC data staged to be output to the DAC
 * @param in_pos Points to starting point of this algorithm chunk
 * @param out_pos Points to starting point of this algorithm chunk
 * @param multisamples The number of input samples per output sample
 */
void algo_test(uint16_t *in_buff, uint8_t *out_buff, uint16_t in_pos, uint16_t out_pos, int multisamples);

/**
 * @brief Initializes algorithm constants
 * @param algo_size The amount of data to process from the input
 */
void algo_test_init(int algo_size); 
