/**
 * algo_template.c
 * Author: Corey Bean,
 * Organization: Messiah Collaboratory
 * Date: 10/12/2020
 *
 * Description:
 * This file demonstrates the template for the algorithm file.
 */

#include "algo_template.h"

void algo_template(uint16_t *adc_buff, uint8_t *dac_buff, uint16_t adc_pos, uint16_t dac_pos, int multisamples) {

    /**
     * adc_pos and dac_pos point to the algorithm half of the buffer. Only ready and write to data in the half
     * of the buffer designated by these positions
     * 
     * The algorithm should have minimum side effects: try not to write to globals defined in other files, etc.
     */

    /* This algorithm simply outputs the input to the ADC */
    for (int i = 0; i < algo_template_size / multisamples; i++) {

        //the following section averages the multisampled ADC data so that each DAC buffer space will have one corresponding ADC value
        uint16_t avg = 0;

        for (int j = 0; i < multisamples; j++) {
            avg += adc_buff[adc_pos + (i * multisamples) + j];
        }

        avg = avg / multisamples;
        /** END SECTION **/
        
        dac_buff[dac_pos + i] = avg >> 4;
    }

}

void algo_template_init(int algo_size) {
    algo_template_size = algo_size;
}