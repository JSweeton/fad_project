/**
 * fad_timer.c
 * Author: Corey Bean,
 * Organization: Messiah Collaboratory
 * Date: 10/03/2020
 *
 * Description:
 * This file initializes the timer and holds the timer alarm interrupt, which gets called at a set frequency
 */

#include "esp_system.h"
#include "driver/timer.h"
#include "esp_intr_alloc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/adc.h"
#include "esp_log.h"
#include "driver/uart.h"

#include "main.h"
#include "fad_adc.h"
#include "fad_dac.h"
#include "fad_defs.h"
#include "fad_app_core.h"

#define TIMER_GROUP TIMER_GROUP_0
#define TIMER_NUMBER TIMER_0
#define CLOCK_DIVIDER (80000000 / TIMER_FREQ) //divider required to make timer frequency correct
#define ALARM_STEP_SIZE (TIMER_FREQ / ALARM_FREQ)
#define TASK_STACK_DEPTH 2048

SemaphoreHandle_t alarmSemaphoreHandle;
xTaskHandle alarmTaskHandle;

static const char *TIMER_TAG = "TIMER";
static bool s_timer_running = 0; 	// keep track of whether timer is on
static int s_adc_read_size = 0;
static fad_output_mode_t s_output_mode = FAD_OUTPUT_BT;

/**
 * @brief Interrupt that is called every time the timer reaches the alarm value. Its purpose
 * is to perform an ADC reading and warn the ADC event handler when the buffer fills up,
 * so that the desired algo function will operate on the data.
 * @param arg Optional arg to be passed to interrupt from timer.
 * @return
 * 		-true if a higher priority task has awoken from handler execution
 * 		-false if no task was awoken
 */
void IRAM_ATTR timer_intr_handler(void *arg)
{

	timer_spinlock_take(TIMER_GROUP); //At beginning and end, Timer API asks us to enclose ISR with spinlock _take and _give functions to function properly

	//advance buffer, resetting to zero at max buffer position
	adc_buffer_pos = adc_buffer_pos + 1; //adc_buffer_pos is global
	if (adc_buffer_pos == ADC_BUFFER_SIZE) adc_buffer_pos = 0;

	// take reading
	adc_buffer[adc_buffer_pos] = local_adc1_read(ADC_CHANNEL);

	if (adc_buffer_pos % MULTISAMPLES == 0)
	{ //wait to increment dac buffer and output only when the multisample number of ADC samples have been taken.
		dac_buffer_pos = (dac_buffer_pos + 1) % DAC_BUFFER_SIZE;
		if (s_output_mode == FAD_OUTPUT_DAC)
		{
			dac_output_value(dac_buffer[dac_buffer_pos]);
		}
	}

	if (adc_buffer_pos % s_adc_read_size == 0)
	{
		adc_buffer_pos_copy = adc_buffer_pos; //these copies provide a stable reference for the algorithm to work on
		dac_buffer_pos_copy = dac_buffer_pos;

		BaseType_t yield = false;	// required for next call
		xSemaphoreGiveFromISR(alarmSemaphoreHandle, &yield);
	}

	timer_group_clr_intr_status_in_isr(TIMER_GROUP, TIMER_NUMBER); // clear the interrupt
	timer_group_enable_alarm_in_isr(TIMER_GROUP, TIMER_NUMBER); // enable alarm
	timer_spinlock_give(TIMER_GROUP);
}

/**
 * @brief FreeRTOS task that runs when the buffer is ready for output. 
 * Adds the appropriate function to the ADC task handler.
 * 
 * @param params [in] required as part of the task function definition
 */
static void alarm_task(void *params)
{

	for (;;)
	{
		xSemaphoreTake(alarmSemaphoreHandle, portMAX_DELAY);

		fad_main_cb_param_t params = {
			.adc_buff_pos_info.adc_pos = adc_buffer_pos_copy,
			.adc_buff_pos_info.dac_pos = dac_buffer_pos_copy,
		};

		fad_app_work_dispatch(fad_main_stack_evt_handler, FAD_ADC_BUFFER_READY, (void *)&params, sizeof(fad_main_cb_param_t), NULL);
	}

	vTaskDelete(alarmTaskHandle);
}

esp_err_t adc_timer_init(void)
{
	//API struct from driver/timer.h
	timer_config_t adc_timer =
		{
			.alarm_en = TIMER_ALARM_EN,
			.counter_en = TIMER_PAUSE,
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
	err = timer_isr_register(TIMER_GROUP, TIMER_NUMBER, timer_intr_handler, 0, ESP_INTR_FLAG_IRAM, NULL);

	alarmSemaphoreHandle = xSemaphoreCreateBinary();
	xTaskCreate(alarm_task, "Interrupt_Alarm_Task_Handler", TASK_STACK_DEPTH,
				0, configMAX_PRIORITIES - 3, &alarmTaskHandle);

	if (err)
		ESP_LOGI(TIMER_TAG, "Error");

	s_timer_running = false;
	return err;
}

esp_err_t adc_timer_start(void)
{
	esp_err_t ret;
	ret = timer_start(TIMER_GROUP, TIMER_NUMBER);
	s_timer_running = true;

	return ret;
}

void adc_timer_pause(void)
{
	s_timer_running = false;
	timer_pause(TIMER_GROUP, TIMER_NUMBER);
}

esp_err_t adc_timer_set_read_size(int adc_read_size)
{
	if (s_timer_running == true) return ESP_FAIL;

	s_adc_read_size = adc_read_size;
	return ESP_OK;
}

esp_err_t adc_timer_set_mode(fad_output_mode_t mode)
{
	if (s_timer_running == true) return ESP_FAIL;

	s_output_mode = mode;
	return ESP_OK;
}