/**
 * fad_adc.c
 * Author: Corey Bean,
 * Organization: Messiah Collaboratory
 * Date: 9/17/2020
 *
 * Description:
 * This file handles ADC input for the user's voice audio data.
 * It uses an ESP32 hardware timer with an interrupt
 */

#include <stdbool.h>
#include <stdlib.h>
#include "driver/adc.h"
#include "hal/adc_types.h"
#include "hal/adc_ll.h"
#include "driver/timer.h"
#include "esp_intr_alloc.h"
#include "esp_log.h"
#include "fad_adc.h"
#include "fad_app_core.h"
#include "fad_dac.h"
#include "fad_defs.h"
#include "fad_algorithms/algo_white.c"
#include "fad_algorithms/algo_test.c"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"



/**
 * Event enumeration for ADC-related events
 */
enum {
	ADC_BUFFER_READY_EVT,
} adc_evt;

typedef union {

	/**
	 * @brief ADC_BUFFER_READY_EVT
	 */
	struct adc_buffer_rdy_param {
		uint16_t adc_pos;
		uint16_t dac_pos;
	} buff_pos;

} adc_evt_params;

#define TIMER_GROUP TIMER_GROUP_0
#define TIMER_NUMBER TIMER_0
#define ALARM_STEP_SIZE 10
#define TIMER_FREQ 80000
#define CLOCK_DIVIDER (80000000 / TIMER_FREQ) //divider required to make timer 

#define STACK_DEPTH 2048

static const char *ADC_TAG = "ADC";

SemaphoreHandle_t alarmSemaphoreHandle;
xTaskHandle alarmTaskHandle;

/**
 * 	Need a circular buffer for the adc input, as well as a tracker for the
 * 	current position in the queue
 */
uint16_t *adc_buffer;
uint8_t *dac_buffer;
uint16_t adc_buffer_pos;
uint16_t dac_buffer_pos;
uint16_t adc_buffer_pos_copy;
uint16_t dac_buffer_pos_copy;
algo_func_t algo_function;

/**
 * @brief	Event handler for ADC-related events
 * @param 	evt An event for the adc
 * @param 	params Optional event parameters if necessary
 */
void adc_hdl_evt(uint16_t evt, void *params) {

	switch (evt) {
	case (ADC_BUFFER_READY_EVT): {
		adc_evt_params *data = (adc_evt_params *) params;
		ESP_LOGD(ADC_TAG, "ADC Value: %d", adc_buffer[0]);
		algo_function(adc_buffer, dac_buffer, data->buff_pos.adc_pos, data->buff_pos.dac_pos);
	}
	}

}

/**
 * @brief	Initialize input buffer for ADC data, as well as DAC buffer for output
 * @return
 * 		-ESP_OK if successful
 * 		-ESP_ERR_NO_MEM if buffers cannot be allocated memory
 */
static esp_err_t adc_buffer_init(void) {
	adc_buffer = (uint16_t*) calloc(ADC_BUFFER_SIZE, sizeof(uint16_t));
	if (adc_buffer == NULL) {
		return ESP_ERR_NO_MEM;
	}

	dac_buffer = (uint8_t*) calloc(ADC_BUFFER_SIZE, sizeof(uint8_t));
	if (dac_buffer == NULL) {
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
static int IRAM_ATTR local_adc1_read(int channel) {
    adc_ll_rtc_enable_channel(ADC_NUM_1, channel);
    adc_ll_rtc_start_convert(ADC_NUM_1, channel);
    while (adc_ll_rtc_convert_is_done(ADC_NUM_1) != true);
    return adc_ll_rtc_get_convert_value(ADC_NUM_1);
}

/**
 * @brief Interrupt that is called every time the timer reaches the alarm value. Its purpose
 * is to perform an ADC reading and warn the ADC event handler when the buffer fills up,
 * so that the desired algo function will operate on the data.
 * @param arg Optional arg to be passed to interrupt from timer.
 * @return
 * 		-true if a higher priority task has awoken from handler execution
 * 		-false if no task was awoken
 */
void IRAM_ATTR adc_timer_intr_handler(void *arg) {
	
	timer_spinlock_take(TIMER_GROUP);
	BaseType_t yield = false;

	//advance buffer, resetting to zero at max buffer position
	adc_buffer_pos = (adc_buffer_pos + 1) % ADC_BUFFER_SIZE;
	dac_buffer_pos = (dac_buffer_pos + 1) % ADC_BUFFER_SIZE;

	adc_buffer[adc_buffer_pos] = local_adc1_read(ADC1_CHANNEL_6);
	dac_output_value(dac_buffer[dac_buffer_pos]);

	if (adc_buffer_pos % adc_algo_size == 0) {
		adc_buffer_pos_copy = adc_buffer_pos; //these copies provide a stable reference for the algorithm to work on
		dac_buffer_pos_copy = dac_buffer_pos;
		xSemaphoreGiveFromISR(alarmSemaphoreHandle, &yield);
		
	}

	//clear the interrupt
	timer_group_clr_intr_status_in_isr(TIMER_GROUP, TIMER_NUMBER);

	timer_spinlock_give(TIMER_GROUP);
	if(yield) taskYIELD();

}

static void alarm_task(void *params) {
	for(;;) {

		xSemaphoreTake(alarmSemaphoreHandle, portMAX_DELAY);

		adc_evt_params params = {
			.buff_pos.adc_pos = adc_buffer_pos_copy,
			.buff_pos.dac_pos = dac_buffer_pos_copy,
		};

		fad_app_work_dispatch(adc_hdl_evt, ADC_BUFFER_READY_EVT, (void *) &params, sizeof(adc_evt_params), NULL);
		// ESP_LOGI(ADC_TAG, "TASK TAKEN!");
	}

	vTaskDelete(alarmTaskHandle);
}

/**
 * @brief 	Initializes the timer for the ADC input. Uses API found in driver/timer.h
 * @return
 * 		-ESP_OK if successful
 * 		-Non-zero if unsuccessful
 */
esp_err_t adc_timer_init(void) {
	//API struct from driver/timer.h
	timer_config_t adc_timer =
			{ .alarm_en = TIMER_ALARM_EN,
				.counter_en = TIMER_PAUSE,
				.intr_type = TIMER_INTR_LEVEL,
				.counter_dir = TIMER_COUNT_UP,
				.auto_reload = TIMER_AUTORELOAD_EN,
				.divider = CLOCK_DIVIDER, //80 MHz / 2000 = 40 kHz
			};

	esp_err_t err;

	//API functions from driver/timer.h
	err = timer_init(TIMER_GROUP, TIMER_NUMBER, &adc_timer);
	err = timer_set_counter_value(TIMER_GROUP, TIMER_NUMBER, 0x00000000ULL);
	err = timer_enable_intr(TIMER_GROUP, TIMER_NUMBER);
	err = timer_set_alarm_value(TIMER_GROUP, TIMER_NUMBER, ALARM_STEP_SIZE);
	err = timer_isr_register(TIMER_GROUP, TIMER_NUMBER, adc_timer_intr_handler, 0, ESP_INTR_FLAG_IRAM, NULL);

	alarmSemaphoreHandle = xSemaphoreCreateBinary();
	xTaskCreate(alarm_task, "Interrupt_Alarm_Task_Handler", STACK_DEPTH,
				0, configMAX_PRIORITIES - 3, &alarmTaskHandle);

	return err;
}

/**
 * @brief	Initializes ADC settings and calls function to initiate timer and buffer
 * @return
 * 		-ESP_OK if successful
 */
esp_err_t adc_init(void) {

	//bit width of ADC input
	adc1_config_width(ADC_WIDTH_BIT_12);

	//attenuation
	adc1_config_channel_atten(ADC1_CHANNEL_6, ADC_ATTEN_DB_0);

	esp_err_t ret;

	ret = adc_buffer_init();

	ret = adc_timer_init();

	algo_white_init(&algo_function);

	return ret;

}

/**
 * @brief	Function to start the timer, meaning interrupts will start occurring
 * @return
 * 		-ESP_OK if successful
 */
esp_err_t adc_timer_start(void) {
	esp_err_t ret;
	ret = timer_start(TIMER_GROUP, TIMER_NUMBER);

	return ret;
}

