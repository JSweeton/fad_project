/**
 * algo_masking.c
 * Author: Corey Bean, Chad Long, Tim Fair
 * Organization: Messiah Collaboratory
 * Date: 10/12/2020
 *
 * Description:
 * This algo imitates the Edinburgh Masker by finding the fft of the input data over a sampled period and
 * using this to create a sawtooth wave at the frequency of the found fundamental frequency.
 */

#include "algo_masking.h"
#include "esp_log.h"
#include "fft.h"
#include "fad_defs.h"
#include "driver/adc.h"
#include "math.h"

static const char* TAG = "algo_masking";

/* Defines how many values the algorithm will read from the ADC buffer. Should always be at least half of buffer size. */
static int s_algo_template_read_size = 2048; // was 512, then 2048

/* References the sampling freq of the adc */
//static int s_algo_sampling_freq = 40000;

/*Determines the time of which data is captured */
//static float s_algo_total_time = 2048/40000;

/* Stores max_magnitude */
//volatile float s_algo_max_magnitude = 0;

/* Stores fundamental frequency */
//volatile float s_algo_fundamental_freq = 0;

/* Stores max_amplitude */
//volatile float s_algo_max_amplitude = 0;

/* Determines the period of the square wave */
//static int s_period = 30;

//static int switching_voltage = 2048;

//float fft_input[2048];

//float fft_output[2048];

//uint8_t val = 0;

float in_signal_count;
float out_signal_count;
bool in_sig_flag = false;
float current_ADC_val;
float threshold = 1024;
float max_out_count;
float roll_AVG = 5;
float DAC_out_val;
float MAX_DAC_OUT = 2048;


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

    //float fft_input[2048];

    //float fft_output[2048];
    
    //fft_config_t *real_fft_plan = fft_init(s_algo_template_read_size, FFT_REAL, FFT_FORWARD, NULL, NULL);

    //val = 0;

     for (int i = 0; i < s_algo_template_read_size; i++){

        in_signal_count++;
        out_signal_count++;

        current_ADC_val = in_buff[in_pos + 1];

        if((in_sig_flag && (current_ADC_val <= threshold)) || (!in_sig_flag && (current_ADC_val > threshold))){

            max_out_count = (max_out_count*((roll_AVG - 1)/roll_AVG)) + (in_signal_count/roll_AVG);

            in_signal_count = 0;
            in_sig_flag = !in_sig_flag;
        }

        DAC_out_val = ((out_signal_count / (2 * max_out_count)) * MAX_DAC_OUT);

        out_buff[out_pos + i] = (int)DAC_out_val;

        if(out_signal_count >= (2*max_out_count)){
            out_signal_count = 0;
        }
        /**
        //Input current input buffer data into dataset used for fft calc
        real_fft_plan->input[i] = in_buff[in_pos + i];
        ESP_LOGI(TAG, "val: %d", in_buff[in_pos + i]);

        //In theory the following code will be passed the correct values from the fft transform:
        val = (val + ((s_algo_max_amplitude * s_algo_fundamental_freq) / s_algo_sampling_freq));
        out_buff[out_pos + i] = val;

        //Execute fft when buffer fills -not working as expected, possibly looping too quickly/fft not executing properly
        if(i == 2047){ //s_algo_template_read_size+1
            fft_execute(real_fft_plan);
            
            for (int k = 1 ; k < real_fft_plan->size / 2 ; k++)
            {
            //The real part of a magnitude at a frequency is 
                //followed by the corresponding imaginary part in the output
            float mag = sqrt(pow(real_fft_plan->output[2*k],2) + pow(real_fft_plan->output[2*k+1],2));
            
            float freq = k*1.0/s_algo_total_time;
                if(mag > s_algo_max_magnitude)
                {
                    s_algo_max_magnitude = mag;
                    //ESP_LOGI(TAG, "magnitude: %f", s_algo_max_magnitude);
                    s_algo_fundamental_freq = freq;
                    //ESP_LOGI(TAG, "freq: %f", s_algo_fundamental_freq);
                    s_algo_max_amplitude = (2/2048) * mag;
                    //ESP_LOGI(TAG, "amplitude: %f", s_algo_max_amplitude);
                }
            }
            //out_buff[i] = s_algo_fundamental_freq; 
        }
        */
      }
     ESP_LOGI(TAG, "in signal count... %0.3f", in_signal_count);
     ESP_LOGI(TAG, "out signal count... %0.3f", out_signal_count);
    //fft_destroy(real_fft_plan);
    //ESP_LOGI(ALGO_TAG, "running algo... %d", out_buff[out_pos]);
}

void algo_masking_init()//fad_algo_init_params_t *params
{
    //s_algo_template_read_size = params->algo_template_params.read_size;
    //s_period = params->algo_template_params.period;
    //float fft_input[s_algo_template_read_size];
    //float fft_output[s_algo_template_read_size];
    //fft_config_t *real_fft_plan = fft_init(s_algo_template_read_size, FFT_REAL, FFT_FORWARD, fft_input, fft_output);
}

void algo_masking_deinit()
{
    // Undo any memory allocations here
}


//Tim Notes: read and write dac into global variables, not a buffer. Start task call to masking algo
//within timer routine, but not a funciton call. 
