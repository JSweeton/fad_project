/**
 * algo_white.c
 * Author: Corey Bean,
 * Organization: Messiah Collaboratory
 * Date: 9/17/2020
 *
 * Description:
 * This file runs through the white noise algorithm for our masker.
 * Bit-wise AND with the input ADC and a pseudo-random number from esp_random 
 */

#include "algo_white.h"
#include "esp_system.h"
#include "esp_log.h"

void algo_white(uint16_t *in_buff, uint8_t *out_buff, uint16_t in_pos, uint16_t out_pos, int multisamples)
{
	/* quick scan for max/min */
	uint16_t max = 0;
	uint16_t min = 0xFFFF;
	for (int i = 0; i < 64; i++)
	{
		max = ( max < in_buff[i] ) ? in_buff[i] : max;
		min = ( min > in_buff[i] ) ? in_buff[i] : min;
	}

	uint16_t diff = (max - min) >> 4; /* Bit shift 4 times to turn 12 bit ADC input to 8 bit char */
	// ESP_LOGI("ALGO", "diff: %d", diff);
	out_buff[0] = 0x7F;


	uint16_t next_avg;
	uint16_t cur_avg;

	for (int i = 0; i < ( algo_white_size / multisamples ) - 1; i++)
	{
		next_avg = in_buff[i + 1];
		cur_avg = in_buff[i];
		/* to center input wave around 0 plus some, we effectively take discrete derivative, then discrete integral */
		int8_t discrete_diff = (int8_t)((next_avg - cur_avg) >> 4);

		out_buff[i + 1] = 0x7F + (((char)esp_random() * discrete_diff) >> 8);
	}
}

void algo_white_init(int algo_size)
{
	algo_white_size = algo_size;
}