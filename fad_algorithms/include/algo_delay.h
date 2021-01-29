/**
 * algo_template.h
 * Author: Corey Bean,
 * Organization: Messiah Collaboratory
 * Date: 10/12/2020
 *
 * Description:
 * This file returns a delayed version of the input signal, shifted by some target amount.
 */

#include <stdint.h>

/* Defines how many values the algorithm will read from the ADC buffer. Should always be at least half of buffer size. */
int algo_delay_read_size_g;

/* Define any globals */
/* This delay size determines how many samples (in terms of output) to shift the incoming signal by. Determines delay time. */
int delay_size_g;

/* This is a circular buffer that holds signals for the output delay. Needs to be allocated based on the sample delay amount. */
uint8_t *delay_buffer_g;

/* This is the current position in the delay buffer. */
int delay_buffer_pos_g;

/**
 * @brief Delay algorithm for ESP masker
 * @param in_buff Buffer that points to the beginning of the ADC data
 * @param out_buff [OUT] Buffer that points to the beggining of DAC data staged to be output to the DAC
 * @param in_pos Points to starting point of this algorithm chunk
 * @param out_pos Points to starting point of this algorithm chunk
 * @param multisamples Number of input samples per output sample
 */
void algo_delay(uint16_t *in_buff, uint8_t *out_buff, uint16_t in_pos, uint16_t out_pos, int multisamples); 

/**
 * @brief Initializes algorithm constants
 * @param algo_size The amount of data to process from the input
 */
void algo_delay_init(int algo_size, int delay_size);

void algo_delay_deinit();