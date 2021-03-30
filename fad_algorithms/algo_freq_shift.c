/**
 * algo_template.c
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


static double algo_freq_read_size_g;

static double pitch_freq;

static double[] shift_array = {};
static double[] input_data = {};



void algo_freq_shift(uint16_t *in_buff, uint8_t *out_buff, uint16_t in_pos, uint16_t out_pos, int multisamples){

    double[] input_data = {};
    double[] output_data = {};

    for (int i = 0; i < ADC_BUFFER_SIZE; i++){

        input_data[i] = in_buff[in_pos + i];
        output_data[i] = input_data * shift_array[i];

        out_buff[i] = output_data[out_pos + i];

        


        

    }

    


    
}







void algo_freq_init(int algo_size, int pitch_freq){
    algo_freq_read_size_g = algo_size;



    for int(i = 0, i < ADC_BUFFER_SIZE, i++){
        shift_array[i] = sin(i * (pitch_freq) / OUPUT_FREQ);
    }

}