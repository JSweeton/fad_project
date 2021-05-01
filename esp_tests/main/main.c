/**
 * main.c
 * Author: Corey Bean
 * 
 * This file is for testing purposes.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "driver/timer.h"
#include "driver/gpio.h"

#define TIMER_GROUP 0
#define TIMER_NUMBER 0
#define DUTY_CYCLE_CHANGE_RATE 1000

TaskHandle_t output_task_handle;

static int s_on_pos = 0;
static int on_cycles = 0;
static int macro_time_track = 0;



esp_err_t adc_timer_init(void);

void IRAM_ATTR timer_intr_handler(void *arg)
{
	timer_spinlock_take(TIMER_GROUP); //At beginning and end, Timer API asks us to enclose ISR with spinlock _take and _give functions to function properly

    if (s_on_pos < on_cycles) {
        gpio_set_level(GPIO_NUM_2, 1);
    }
    else 
    {
        gpio_set_level(GPIO_NUM_2, 0);
    }

    if (++s_on_pos == 10) {
        s_on_pos = 0;
    }

    macro_time_track++;
    if (macro_time_track == DUTY_CYCLE_CHANGE_RATE)
    {
        on_cycles++;
        if (on_cycles == 10) on_cycles = 0;
        macro_time_track = 0;
    }

	timer_group_clr_intr_status_in_isr(TIMER_GROUP, TIMER_NUMBER); // clear the interrupt
	timer_group_enable_alarm_in_isr(TIMER_GROUP, TIMER_NUMBER); // enable alarm
	timer_spinlock_give(TIMER_GROUP);
}

void app_main(void)
{
    gpio_config_t conf = {
        
        .pin_bit_mask = 1 << GPIO_NUM_2,          /*!< GPIO pin: set with bit mask, each bit maps to a GPIO */
        .mode = GPIO_MODE_OUTPUT,               /*!< GPIO mode: set input/output mode                     */
        .pull_up_en = 0,       /*!< GPIO pull-up                                         */
        .pull_down_en = 0,   /*!< GPIO pull-down                                       */
        .intr_type = GPIO_INTR_DISABLE,      /*!< GPIO interrupt type                                  */

    };
    gpio_config(&conf);

    gpio_set_level(GPIO_NUM_2, 0);

    adc_timer_init();
    // xTaskCreate(output_task, "LED_OUTPUT_TASK", 2048, NULL, configMAX_PRIORITIES - 2, &output_task_handle);
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
			.divider = 2000, //80 MHz / 2000 = 40 kHz
		};

	esp_err_t err;

	//API functions from driver/timer.h
	err = timer_init(TIMER_GROUP, TIMER_NUMBER, &adc_timer);
	err = timer_set_counter_value(TIMER_GROUP, TIMER_NUMBER, 0x00000000ULL);
	err = timer_enable_intr(TIMER_GROUP, TIMER_NUMBER);
	err = timer_set_alarm_value(TIMER_GROUP, TIMER_NUMBER, 10);
	err = timer_isr_register(TIMER_GROUP, TIMER_NUMBER, timer_intr_handler, 0, ESP_INTR_FLAG_IRAM, NULL);
    timer_start(TIMER_GROUP, TIMER_NUMBER);

	return err;
}