#include <driver/adc.h>





void adc_init(void) {

	//bit width of ADC input
    adc1_config_width(ADC_WIDTH_BIT_12);

    //attenuation
    adc1_config_channel_atten(ADC1_CHANNEL_0,ADC_ATTEN_DB_0);

}

void adc_timer_init(void) {

}
