/**
 * fad_dac.c
 * Author: Corey Bean,
 * Organization: Messiah Collaboratory
 * Date: 9/17/2020
 *
 * Description:
 * This file initializes and handles functions for the DAC output
 */

#include <stdlib.h>
#include "fad_dac.h"
#include "driver/dac.h"
#include "esp_system.h"

#define DAC_CHANNEL DAC_CHANNEL_1

/**
 * @brief Initialize DAC through Espressif DAC API
 */
esp_err_t dac_init(void) {
	esp_err_t err = dac_output_enable(DAC_CHANNEL);
	return err;
}

/**
 * @brief Output given 8 bit value to DAC for the defined DAC_CHANNEL in fad_dac.c
 * @param dac_buffer Pointer to the dac_buffer, which holds the set of dac values to be output
 * @param dac_buffer_pos Current position of next ouput in dac_buffer
 */
void dac_output(uint8_t *dac_buffer, uint16_t dac_buffer_pos) {
	dac_output_voltage(DAC_CHANNEL, dac_buffer[dac_buffer_pos]);

}
