/**
 * algo_template.c
 * Author: Corey Bean,
 * Organization: Messiah Collaboratory
 * Date: 10/12/2020
 *
 * Description:
 * This file demonstrates the template for the algorithm file. It simply outputs a square wave of a varied period.
 */

#include "algo_masking.h"
#include "esp_log.h"

#define ALGO_TAG "ALGO_MASKING"

/* Defines how many values the algorithm will read from the ADC buffer. Should always be at least half of buffer size. */
static int s_algo_template_read_size = 512;

/* Determines the period of the square wave */
static int s_period = 30;

static int switching_voltage = 2048;

void algo_masking(uint16_t *in_buff, uint8_t *out_buff, uint16_t in_pos, uint16_t out_pos, int multisamples)
{

    /**
     * in_pos and out_pos point to the algorithm half of the buffer. Only ready and write to data in the half
     * of the buffer designated by these positions
     * 
     * The algorithm should have minimum side effects: try not to write to globals defined in other files, etc.
     */

    /* This algorithm simply outputs the input to the ADC */

    //ESP_LOGI(ALGO_TAG, "s_algo_template_read_size = %d", s_algo_template_read_size);

    for (int i = 0; i < s_algo_template_read_size / multisamples; i++)
    {
        //uint16_t avg = 0;
        //the following section averages the multisampled ADC data so that each DAC buffer space will have one corresponding ADC value

        //for (int j = 0; j < multisamples; j++)
        //{
         //   avg += in_buff[in_pos + (i * multisamples) + j];
        //}

        //avg = avg / multisamples;
        /** END SECTION **/

        //uint8_t val = 0;
        //if ((i / s_period) % 2 == 0) val = (avg >> 5) + 100; // Generates square wave with freq of 11k / period
        
        uint8_t val = in_buff[in_pos + i];
        out_buff[out_pos + i] = val;

    }
     ESP_LOGI(ALGO_TAG, "running algo... %d", out_buff[out_pos]);
}

void algo_masking_init(fad_algo_init_params_t *params)
{
    s_algo_template_read_size = params->algo_template_params.read_size;
    s_period = params->algo_template_params.period;
}

void algo_masking_deinit()
{
    // Undo any memory allocations here
}