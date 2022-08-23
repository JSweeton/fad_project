/**
 * algo_freq_shift.c
 * Author: Larry Vega,
 * Organization: Messiah Collaboratory
 * Date: 3/25/2021
 *
 * Description:
 * This file returns the input signal at a different pitch, shifted by some target amount.
 */



#include "algo_freq_shift.h"
#include "fad_defs.h"
#include <stdlib.h>
#include <math.h>


static int algo_freq_read_size_g = 512;

static int pitch_freq = 2;


//static int8_t[] shift_array_s;
static int shift_array_period_s = 0;
static int shift_array_pos_s = 0;

void algo_freq_shift(uint16_t *in_buff, uint8_t *out_buff, uint16_t in_pos, uint16_t out_pos, int multisamples)
{
    /* Need to multiply incoming data by sine wave to shift frequency. However, incoming data is centered around 2048 (ideally). 
    If you simply multiply these together, there will be a leftover signal from the huge DC component of the incoming data.
    You have to figure out how to mitigate this. Possible solution is to..
        Subtract DC component from each sample before multiplying by the current sine position, then re-add the DC component
            Considerations: Make sure the resulting DAC output value won't be negative after re-adding the DC component, because
            the negative value will show up as a large positive value when it is converted back to an unsigned int.
            */
    
    /* e.g.
    for (int i = 0; i < algo_freq_read_size_g, i++) 
    {
        int val = in_buff[in_pos + i];
        // shifting down
        val -= 2048;
        val = val * shift_array_s[shift_array_pos_s++];

        // manage circular buffer
        if (shift_array_pos_s == shift_array_period_s) 
        {
            shift_array_pos_s = 0;
        }

        // divide output to fit in DAC BEFORE re-adding DC component
        val = val >> 10 // (not sure if 10 is correct)
        val += 128;
        out_buff[out_pos + i] = val;
        
    }
    */

    for (int i = 0; i < ADC_BUFFER_SIZE; i++)
    {
        /*
        input_data[i] = in_buff[in_pos + i];
        output_data[i] = input_data * shift_array[i];

        out_buff[i] = output_data[out_pos + i];
        */
       in_buff[in_pos + i]=out_buff[out_pos+i];


    }    
    
}


void algo_freq_init()
//void algo_freq_init(fad_algo_init_params_t *params)
{
    //algo_freq_read_size_g = params->algo_freq_shift_params.read_size;
    //int pitch_freq = params->algo_freq_shift_params.shift_amount;
    /*
    int period = ALARM_FREQ / pitch_freq;
    shift_array_period_s = period;

    shift_array = (uint32_t *) malloc(period * sizeof(uint32_t));

    for (double i = 0; i < period, i++)
    {
        shift_array[i] = (int8_t) (127 * sin(i * 6.283 / period));
    }
    */
}

void algo_freq_deinit()
{
    //free(shift_array);
}