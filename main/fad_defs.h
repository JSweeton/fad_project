#include <stdlib.h>
#include "esp_system.h"

/*
 * Author: Corey Bean
 * Organization: Messiah Collaboratory
 * Date: 9/17/2020
 */

/*
 * This file will hold all of the constants for our project. You can change their
 * values directly here.
 */

/*
 * ADC Definitions
 */

//Buffer size for holding ADC data
#define ADC_BUFFER_SIZE 1024



/*
 * Global variables
 */

//determines the chunck size fed to the algorithm
uint16_t adc_algo_size = 0;

