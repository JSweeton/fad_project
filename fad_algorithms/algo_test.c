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
#include "esp_log.h"

void algo_test(uint16_t *in_buff, uint8_t *out_buff, uint16_t in_pos, uint16_t out_pos, int multisamples)
{
    for (int i = 0; i < algo_test_size / multisamples; i++) {
        out_buff[out_pos + i] = in_buff[in_pos + (i * multisamples)] >> 4;
    }
}

void algo_test_init(int algo_size)
{
    algo_test_size = algo_size;
}
