/**
 * algo_template.h
 * Author: Corey Bean,
 * Organization: Messiah Collaboratory
 * Date: 10/12/2020
 *
 * Description:
 * This file demonstrates the template for the algorithm file.
 */

#include <stdint.h>

/* Defines how many values the algorithm will read from the ADC buffer. Should always be at least half of buffer size. */
int algo_template_size;

/* Define any globals */

/**
 * @brief White noise algorithm for ESP masker
 * @param in_buff Buffer that points to the beginning of the ADC data
 * @param out_buff [OUT] Buffer that points to the beggining of DAC data staged to be output to the DAC
 * @param in_pos Points to starting point of this algorithm chunk
 * @param out_pos Points to starting point of this algorithm chunk
 * @param multisamples Number of input samples per output sample
 */
void algo_template(uint16_t *in_buff, uint8_t *out_buff, uint16_t in_pos, uint16_t out_pos, int multisamples); 

/**
 * @brief Initializes algorithm constants
 * @param algo_size The amount of data to process from the input
 */
void algo_template_init(int algo_size);