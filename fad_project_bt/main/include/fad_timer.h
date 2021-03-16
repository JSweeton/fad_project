#ifndef _FAD_TIMER_H_
#define _FAD_TIMER_H_

#include "esp_err.h"


/**
 * @brief 	Initializes the timer for the ADC input. Uses API found in driver/timer.h
 * @return
 * 		-ESP_OK if successful
 * 		-Non-zero if unsuccessful
 */
esp_err_t adc_timer_init(void);

/**
 * @brief	Function to start the timer, meaning interrupts will start occurring
 * @return
 * 		-ESP_OK if successful
 */
esp_err_t adc_timer_start(void);

/**
 * @brief Stop timer routine
 */
void adc_timer_stop(void);

/**
 * @brief Set how many readings the timer should take before the data is sent to the algorithm.
 * Should not be called if the timer is running (will return error)
 * 
 * @param adc_read_size The desired number of readings
 * @return
 *      -ESP_OK if successful
 *      -ESP_FAIL if unsuccessful, timer running
 */
esp_err_t adc_timer_set_read_size(int adc_read_size);

/**
 * @brief Sets the output mode for the timer. Should not be called if timer is running.
 * @param mode The output mode; either FAD_OUTPUT_BT or FAD_OUTPUT_DAC
 * @return
 *      -ESP_OK if successful
 *      -ESP_FAIL if unsuccessful, timer running
 */
esp_err_t adc_timer_set_mode(fad_output_mode_t mode);

#endif