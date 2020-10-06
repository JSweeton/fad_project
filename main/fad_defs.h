/*
 * Author: Corey Bean
 * Organization: Messiah Collaboratory
 * Date: 9/17/2020
 * 
 * This file will hold all of the constants for our project. You can change their values directly here
 */

#ifndef _FAD_DEFS_H_
#define _FAD_DEFS_H_

#include <stdlib.h>
#include "esp_system.h"



/*
 * ADC Definitions
 */
#define ADC_BUFFER_SIZE 1024    //Buffer size for holding ADC data. 
#define MULTISAMPLES 2          //Number of ADC samples per DAC output
#define DAC_BUFFER_SIZE (ADC_BUFFER_SIZE / MULTISAMPLES)  //Buffer size for holding staged DAC data. Hold one DAC sample for each ADC multisample
#define ADC_CHANNEL ADC_CHANNEL_6



/*
* Function typedefs
*/

/**
 * @brief     algorithm function
 */
typedef void (* algo_func_t) (uint16_t *adc_buff, uint8_t *dac_buff, uint16_t adc_pos, uint16_t dac_pos);

/**
 * @brief     callback function for app events
 */
typedef void (* fad_app_cb_t) (uint16_t event, void *param);

/*
 * Global variables
 */
uint16_t adc_algo_size; //determines the chunck size of ADC values fed to the algorithm, not accounting for multisampling
char *ALGO_TAG;

// 	Need a circular buffer for the adc input, as well as a tracker for the
//  current position in the queue
uint16_t *adc_buffer;
uint8_t *dac_buffer;
uint16_t adc_buffer_pos;
uint16_t dac_buffer_pos;
uint16_t adc_buffer_pos_copy;
uint16_t dac_buffer_pos_copy;
algo_func_t algo_function;



#endif

