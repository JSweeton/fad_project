/**
 * fad_adc.c
 * Author: Corey Bean,
 * Organization: Messiah Collaboratory
 * Date: 9/17/2020
 *
 * Description:
 * This file handles ADC input for the user's voice audio data.
 */

#include <stdbool.h>
#include <stdlib.h>
#include "driver/adc.h"
#include "hal/adc_types.h"
#include "hal/adc_ll.h"
#include "esp_log.h"

#include "main.h"
#include "fad_adc.h"
#include "fad_app_core.h"
#include "fad_dac.h"
#include "fad_defs.h"
#include "fad_timer.h"

static const char *ADC_TAG = "ADC";

/**
 * @brief	Initialize input buffer for ADC data, as well as DAC buffer for output
 * @return
 * 		-ESP_OK if successful
 * 		-ESP_ERR_NO_MEM if buffers cannot be allocated memory
 */
static esp_err_t adc_buffer_init(void)
{
	adc_buffer = (uint16_t *)calloc(ADC_BUFFER_SIZE, sizeof(uint16_t));
	if (adc_buffer == NULL)
	{
		return ESP_ERR_NO_MEM;
	}

	dac_buffer = (uint8_t *)calloc(ADC_BUFFER_SIZE, sizeof(uint8_t));
	if (dac_buffer == NULL)
	{
		return ESP_ERR_NO_MEM;
	}

	adc_buffer_pos = 0;
	dac_buffer_pos = ADC_BUFFER_SIZE - adc_algo_size;

	return ESP_OK;
}

/**
 * @brief Mimic of ESP-IDF's ADC converter code. Needed to be replicated to utulize IRAM_ATTR and for max speed.
 * See toptal.com/embedded/esp32-audio-sampling for more information
 * @param channel The channel to be read from
 * @return The value read from the ADC
 */
int IRAM_ATTR local_adc1_read(int channel)
{
	adc_ll_rtc_enable_channel(ADC_NUM_1, channel);
	adc_ll_rtc_start_convert(ADC_NUM_1, channel);
	while (adc_ll_rtc_convert_is_done(ADC_NUM_1) != true)
		;
	return adc_ll_rtc_get_convert_value(ADC_NUM_1);
}

/**
 * @brief	Initializes ADC settings and calls function to initiate timer and buffer
 * @return
 * 		-ESP_OK if successful
 */
esp_err_t adc_init()
{

	//bit width of ADC input
	adc1_config_width(ADC_WIDTH_BIT_12);

	//attenuation
	adc1_config_channel_atten(ADC_CHANNEL, ADC_ATTEN_DB_0);

	esp_err_t ret;

	ret = adc_buffer_init();

	adc1_get_raw(ADC_CHANNEL);

	return ret;
}
