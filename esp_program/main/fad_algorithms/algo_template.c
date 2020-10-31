/**
 * algo_template.c
 * Author: Corey Bean,
 * Organization: Messiah Collaboratory
 * Date: 10/12/2020
 *
 * Description:
 * This file demonstrates the template for the algorithm file.
 */

#include "fad_defs.h"
#include "esp_log.h"

/* Defines how many values the algorithm will read from the ADC buffer. Should always be at least half of buffer size. */
#define ALGO_TEMPLATE_SIZE (ADC_BUFFER_SIZE / 2)

/* Define any globals */

/**
 * @brief White noise algorithm for ESP masker
 * @param adc_buff Buffer that points to the beginning of the ADC data
 * @param dac_buff [OUT] Buffer that points to the beggining of DAC data staged to be output to the DAC
 * @param adc_pos Points to starting point of this algorithm chunk
 * @param dac_pos Points to starting point of this algorithm chunk
 */
void algo_template(uint16_t *adc_buff, uint8_t *dac_buff, uint16_t adc_pos, uint16_t dac_pos) {

    /**
     * adc_pos and dac_pos point to the algorithm half of the buffer. Only ready and write to data in the half
     * of the buffer designated by these positions
     * 
     * The algorithm should have minimum side effects: try not to write to globals defined in other files, etc.
     */

    /* This algorithm simply outputs the input to the ADC */
    for (int i = 0; i < ALGO_TEMPLATE_SIZE / MULTISAMPLES; i++) {

        //the following section averages the multisampled ADC data so that each DAC buffer space will have one corresponding ADC value
        uint16_t avg = 0;

        for (int j = 0; i < MULTISAMPLES; j++) {
            avg += adc_buff[adc_pos + (i * MULTISAMPLES) + j];
        }

        avg = avg / MULTISAMPLES;
        /** END SECTION **/
        
        dac_buff[dac_pos + i] = avg >> 4;
    }

}

/**
 * @brief Call this function from the main function to select this algorithm dynamically.
 */
void algo_template_init() {
	ALGO_TAG = "ALGO_TEMPLATE";
	adc_algo_size = ALGO_TEMPLATE_SIZE;
	algo_function = algo_template; //fad_defs.h
    }
