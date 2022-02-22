/**
 * algo_template.c
 * Author: Corey Bean,
 * Organization: Messiah Collaboratory
 * Date: 10/12/2020
 *
 * Description:
 * This file demonstrates the template for the algorithm file.
 */

#include "algo_delay.h"
#include <stdlib.h>
#include <string.h>
#include "fad_defs.h"

/* Defines how many values the algorithm will read from the ADC buffer. Should always be at least half of buffer size. */
int algo_delay_read_size_g = 512;

/* Define any globals */
/* This delay size determines how many samples (in terms of output) to shift the incoming signal by. Determines delay time. */
int delay_size_g = 1;

/* This is a circular buffer that holds signals for the output delay. Needs to be allocated based on the sample delay amount. */
uint8_t *delay_buffer_g;

/* This is the current position in the delay buffer. */
int delay_buffer_pos_g;

void algo_delay(uint16_t *in_buff, uint8_t *out_buff, uint16_t in_pos, uint16_t out_pos, int multisamples) {

    for(int i = 0; i < (algo_delay_read_size_g) / multisamples; i++)
    {
        out_buff[out_pos + i] = delay_buffer_g[delay_buffer_pos_g];
        
        /* Get average of last multisample values from adc */
        // int multisample_average = 0;
        // for (int j = 0; j < multisamples; j++) {
        //     multisample_average += in_buff[in_pos + (multisamples * i) + j];
        // }

        // /* Actual average division */
        // multisample_average /= multisamples;

        delay_buffer_g[delay_buffer_pos_g] = in_buff[in_pos + i] >> 4; //multisample_average;

        // if (i % 10 == 0) delay_buffer_g[delay_buffer_pos_g] = 255;

        delay_buffer_pos_g++;

        if (delay_buffer_pos_g == delay_size_g) delay_buffer_pos_g = 0;

    }

}

void algo_delay_init() { //fad_algo_init_params_t *params
    //algo_delay_read_size_g = params->algo_delay_params.read_size;
    //delay_size_g = params->algo_delay_params.delay;
    delay_buffer_g = malloc(sizeof(uint8_t) * delay_size_g);
    memset(delay_buffer_g, 128, delay_size_g);
    delay_buffer_pos_g = 0;
}

void algo_delay_deinit() {
    free(delay_buffer_g);
}