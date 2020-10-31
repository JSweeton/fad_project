/**
 * fad_dac.c
 * Author: Corey Bean,
 * Organization: Messiah Collaboratory
 * Date: 9/17/2020
 *
 * Description:
 * This file initializes and handles functions for the DAC output. Need custom DAC function for quick DAC output.
 */

#include <stdlib.h>
#include "fad_dac.h"

#include "driver/dac.h"
#include "soc/dac_periph.h"
#include "hal/dac_types.h"
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
 * @brief Output voltage with value (8 bit).
 * @param channel DAC channel num.
 * @param value Output value. Value range: 0 ~ 255.
 *        The corresponding range of voltage is 0v ~ VDD3P3_RTC.
 */
void IRAM_ATTR dac_output_value(uint8_t value) {
    if (DAC_CHANNEL == DAC_CHANNEL_1) {
        SENS.sar_dac_ctrl2.dac_cw_en1 = 0;
        RTCIO.pad_dac[DAC_CHANNEL].dac = value;
    } else if (DAC_CHANNEL == DAC_CHANNEL_2) {
        SENS.sar_dac_ctrl2.dac_cw_en2 = 0;
        RTCIO.pad_dac[DAC_CHANNEL].dac = value;
    }
}
