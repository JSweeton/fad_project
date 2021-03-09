#ifndef _FAD_ADC_H_
#define _FAD_ADC_H_



/**
 * Function Prototypes
 */

esp_err_t adc_init(); //initializes ADC parameters
int IRAM_ATTR local_adc1_read(int channel);

#endif