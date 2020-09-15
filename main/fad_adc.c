#include <stdbool.h>
#include <stdlib.h>
#include "driver/adc.h"
#include "driver/timer.h"
#include "esp_intr_alloc.h"
#include "esp_log.h"
#include "fad_adc.h"
#include "fad_app_core.h"

enum {
	ADC_BUFFER_READY_EVT,
};

#define TIMER_GROUP TIMER_GROUP_0
#define TIMER_NUMBER TIMER_0
#define ADC_BUFFER_SIZE 1024

static const char *ADC_TAG = "ADC";

/*
 * 	Need a circular buffer for the adc input, as well as a tracker for the
 * 	current position in the queue
 */

uint16_t *adc_buffer;
uint8_t adc_buffer_pos;



bool adc_timer_intr_handler(void *arg) {

	bool yield = false;

	//Need to call ADC API to gather data input and place it in the buffer. Should add
	//"n" amount of ADC data to queue and

	if (adc_buffer_pos % 1024) {
		fad_app_work_dispatch(adc_hdl_evt, ADC_BUFFER_READY_EVT, NULL, 0, NULL);
	}

	adc_buffer_pos++;

	//after ADC_BUFFER_SIZE length data, push semaphore to task handler

	return yield;
}

void adc_hdl_evt(uint16_t evt, void *params) {

	switch (evt) {
	case (ADC_BUFFER_READY_EVT): {
		ESP_LOGI(ADC_TAG, "Buffer ready");
	}
	}

}

esp_err_t adc_init(void) {

	//bit width of ADC input
    adc1_config_width(ADC_WIDTH_BIT_12);

    //attenuation
    adc1_config_channel_atten(ADC1_CHANNEL_0,ADC_ATTEN_DB_0);

    esp_err_t ret;

    ret = adc_buffer_init();

    ret = adc_timer_init();

    return ret;


}

esp_err_t adc_timer_init(void) {

	timer_config_t adc_timer = {
			.alarm_en = TIMER_ALARM_EN,
			.counter_en = TIMER_PAUSE,
			.intr_type = TIMER_INTR_LEVEL,
			.counter_dir = TIMER_COUNT_UP,
			.auto_reload = TIMER_AUTORELOAD_EN,
			.divider = 20000, //80 MHz / 20000 = 4 kHz
	};


	esp_err_t err;

	err = timer_init(TIMER_GROUP, TIMER_NUMBER, &adc_timer);
	err = timer_set_counter_value(TIMER_GROUP, TIMER_NUMBER, 0x00000000ULL);
	err = timer_enable_intr(TIMER_GROUP, TIMER_NUMBER);
	err = timer_set_alarm_value(TIMER_GROUP, TIMER_NUMBER, 10000);

	err = timer_isr_callback_add(
			TIMER_GROUP,
			TIMER_NUMBER,
			adc_timer_intr_handler,
			0,
			ESP_INTR_FLAG_LOWMED
		);

	return err;

}

esp_err_t adc_timer_start(void) {
	esp_err_t ret;
	ret = timer_start(TIMER_GROUP, TIMER_NUMBER);

	return ret;
}

esp_err_t adc_buffer_init(void) {
	adc_buffer = (uint16_t *) calloc(ADC_BUFFER_SIZE, sizeof(uint16_t));
	if(adc_buffer == NULL) {
		return ESP_ERR_NO_MEM;
	}
	adc_buffer_pos = 0;

	return ESP_OK;
}
