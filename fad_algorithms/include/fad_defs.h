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

/* ADC Definitions */
#define ADC_BUFFER_SIZE 2048    //Buffer size for holding ADC data. 
#define MULTISAMPLES 2          //Number of ADC samples per DAC output
#define DAC_BUFFER_SIZE (ADC_BUFFER_SIZE / MULTISAMPLES)  //Buffer size for holding staged DAC data. Hold one DAC sample for each ADC sample divided by multisamples
#define ADC_CHANNEL ADC_CHANNEL_6

/* Timer Definitions */
#define TIMER_FREQ 80000    //Frequency of the Timer
#define ALARM_FREQ 16000    //Determines the frequency of ADC sampling and DAC output


typedef enum {
    FAD_ALGO_DELAY,
    FAD_ALGO_TEMPLATE,
    FAD_ALGO_FREQ_SHIFT,
    FAD_ALGO_PLL,
} algo_type_t;

typedef union {
    /* FAD_ALGO_DELAY */

    /* FAD_ALGO_TEMPLATE */
    struct algo_template_params_t {
        int read_size;
    } algo_template_params;
    

} algo_params_t;

/*
* Function typedefs
*/

/**
 * @brief     algorithm function
 * 
 * @param in_buff Pointer to beginning of ADC buffer (also a global)
 * @param out_buff [OUT] Pointer to beginning of DAC buffer (also a global)
 * @param in_pos Integer index of algorithm beginning location in ADC buffer
 * @param out_pos Integer index of algorithm beginning location in ADC buffer
 * @param multisamples The number of input samples per output
 */
typedef void (* algo_func_t) (uint16_t *in_buff, uint8_t *out_buff, uint16_t in_pos, uint16_t out_pos, int multisamples);

typedef void (* algo_init_func_t) (algo_params_t *params);

typedef void (* algo_deinit_func_t) ();

/**
 * @brief     callback function for app events
 */
typedef void (* fad_app_cb_t) (uint16_t event, void *param);

/*
 * Global variables
 */
char *ALGO_TAG;

uint16_t *adc_buffer;
uint8_t *dac_buffer;
uint16_t adc_buffer_pos;
uint16_t dac_buffer_pos;
uint16_t adc_buffer_pos_copy;
uint16_t dac_buffer_pos_copy;



#endif

