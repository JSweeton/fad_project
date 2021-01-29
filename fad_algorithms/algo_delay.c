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

void algo_delay_init(int algo_size, int delay_size) {
    algo_delay_read_size_g = algo_size;
    delay_size_g = delay_size;
    delay_buffer_g = calloc(delay_size, sizeof(uint8_t));
    delay_buffer_pos_g = 0;
}

void algo_delay_deinit() {
    free(delay_buffer_g);
}