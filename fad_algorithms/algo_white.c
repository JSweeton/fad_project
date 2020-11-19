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

void algo_white(uint16_t *in_buff, uint8_t *out_buff, uint16_t in_pos, uint16_t out_pos, int multisamples)
{
	uint16_t d_pos = out_pos;
	uint16_t a_pos = in_pos;

	/* quick scan for max/min */
	uint16_t max = 0;
	uint16_t min = 0xFFFF;
	for (int i = 0; i < 64; i++)
	{
		max = ( max < in_buff[a_pos + i] ) ? in_buff[a_pos + i] : max;
		min = ( min > in_buff[a_pos + i] ) ? in_buff[a_pos + i] : min;
	}
	uint16_t diff = (max - min) >> 4; /* Bit shift 4 times to turn 12 bit ADC input to 8 bit char */
	out_buff[d_pos] = 0x7F;

	/* next_avg and cur_avg hold current and next set of multisampled ADC values, averaged together. Should be 12 bit unsigned */
	uint16_t next_avg = ( (in_buff[in_pos] + in_buff[in_pos + 1]) >> 1); /* bit shift 1 to limit output to 12 bit */
	uint16_t cur_avg;

	for (int i = 0; i < ( algo_white_size / multisamples ) - 1; i++)
	{
		cur_avg = next_avg;

		/** TODO: FIX AVERAGING SCHEME TO AVERAGE MULTISAMPLES FOR MORE THAN 2 **/
		next_avg = (in_buff[in_pos + (i * 2) + 2] + in_buff[in_pos + (i * 2) + 3]) >> 1;

		/* to center input wave around 0 plus some, we effectively take discrete derivative, then discrete integral */
		int8_t discrete_diff = (int8_t)((next_avg - cur_avg) >> 4);

		out_buff[out_pos + i + 1] = 0x7F + (((char)esp_random() * discrete_diff) >> 8);
	}
}

void algo_white_init(int algo_size)
{
	algo_white_size = algo_size;
}