/**
 * algo_masking.c
 * Author: Tim Fair, James Lim
 * Organization: Messiah Collaboratory
 * Date: 10/12/2020
 *
 * Description:
 * This algo imitates the Edinburgh Masker by creating a triangular wave at the frequency of the user's voice signal.
 */

#include "algo_masking.h"
#include "esp_log.h"
#include "fft.h"
#include "fad_defs.h"
#include "driver/adc.h"
#include "math.h"

static const char* TAG = "algo_masking";

/* Defines how many values the algorithm will read from the ADC buffer. Should always be at least half of buffer size. */
static int s_algo_template_read_size = 1024; // was 512, then 2048

float in_signal_count; //Increments for each sample taken
float out_signal_count; //Increments for each sample taken
bool in_sig_flag = true;  //Flag to indicate whether input signal is increasing or decreasing
float current_ADC_val; // The ADC value for the current sample
float threshold = 2048;  // The middle range of input values from the ADC
float max_out_count=250; // Average frequency value of input. Tracks with the users voice
float roll_AVG = 15; // Average over the last 15 ADC samples to determine frequency
float DAC_out_val; // Output value to the DAC
float MAX_DAC_OUT = 2048; // Maximum value that the DAC can output


void algo_masking(uint16_t *in_buff, uint8_t *out_buff, uint16_t in_pos, uint16_t out_pos, int multisamples)
{

    /**
     * in_pos and out_pos point to the algorithm half of the buffer. Only ready and write to data in the half
     * of the buffer designated by these positions
     * 
     * The algorithm should have minimum side effects: try not to write to globals defined in other files, etc.
     */
     
     for(int i = 0; i < s_algo_template_read_size; i++) //This loop is the point of our troubles
     {

        
        if((i%15)==0) //scales the in signal count //v1.0 = 15
        {    
           in_signal_count++;
        }
          
        out_signal_count++;

        current_ADC_val = in_buff[in_pos + 1]; 
        
        if((in_sig_flag && (current_ADC_val <= threshold)) || (!in_sig_flag && (current_ADC_val > threshold)))
        {
            max_out_count = (max_out_count*((roll_AVG - 1)/roll_AVG)) + ((in_signal_count)/(roll_AVG));    
            in_signal_count = 0;    
            in_sig_flag = !in_sig_flag;
        }

        DAC_out_val = ((((out_signal_count) / (2*max_out_count))) * MAX_DAC_OUT);
        
        out_buff[out_pos + i] = (int)DAC_out_val;

        if(out_signal_count >= (2*max_out_count)){
            out_signal_count = 0;
        }
        

       
      }

           

    ESP_LOGI(TAG, "in signal count... %0.3f", in_signal_count);
     ESP_LOGI(TAG, "out signal count... %0.3f", out_signal_count);
     ESP_LOGI(TAG, "max out count... %0.3f", max_out_count);
     ESP_LOGI(TAG, "currentADC... %0.3f", current_ADC_val);
     ESP_LOGI(TAG, "Roll Average... %0.3f", roll_AVG);
    ESP_LOGI(TAG, "finale... %0.3f", test);

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




