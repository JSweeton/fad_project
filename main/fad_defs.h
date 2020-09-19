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

#define ADC_BUFFER_SIZE 1024 //Buffer size for holding ADC data



/*
 * Global variables
 */

uint16_t adc_algo_size; //determines the chunck size fed to the algorithm

/*
* Function typedefs
*/

/**
 * @brief     algorithm function
 */
typedef void (* algo_func_t) (uint16_t *adc_buff, uint8_t *dac_buff);

/**
 * @brief     callback function for app events
 */
typedef void (* fad_app_cb_t) (uint16_t event, void *param);

#endif

