#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "esp_log.h"
#include "driver/gpio.h"

#include "fad_app_core.h"
#include "fad_defs.h"
#include "main.h"

#define BUTTON_BIT_MASK ((1ULL << FAD_VOL_DN_BN) | (1ULL << FAD_VOL_UP_BN) | (1ULL << FAD_DISC_BN))

/* Stores the number of consecutive button hits to keep track of long presses */
typedef struct
{
    uint16_t volume_up;
    uint16_t volume_down;
    uint16_t wired_detect;
    uint16_t discovery_en;
} gpio_history;


static TimerHandle_t s_poll_handle;
static const char *GPIO_TAG = "GPIO";
static gpio_history s_gpio_history = {0, 0, 0, 0};

void gpio_polling_task(TimerHandle_t timer)
{
    s_gpio_history.volume_down = gpio_get_level(FAD_VOL_DN_BN) ? s_gpio_history.volume_down + 1 : 0;
    s_gpio_history.volume_up = gpio_get_level(FAD_VOL_UP_BN) ? s_gpio_history.volume_up + 1 : 0;
    ESP_LOGI(GPIO_TAG, "Polling Task, vup: %d, vdn: %d", s_gpio_history.volume_up, s_gpio_history.volume_down);
}

void fad_gpio_init()
{
    // Configure buttons
    gpio_config_t button_config;
    button_config.pin_bit_mask = BUTTON_BIT_MASK;
    button_config.intr_type = GPIO_INTR_DISABLE;
    button_config.pull_up_en = 1;
    button_config.pull_down_en = 0;
    button_config.mode = GPIO_MODE_INPUT;

    // esp_err_t err = gpio_config(&button_config);
    
    // Configure Headset detection
    /* [TODO] */

    // Initialize button checking timer
    long timer_id;
    s_poll_handle = xTimerCreate("Polling Timer", pdMS_TO_TICKS(FAD_GPIO_POLLING_PERIOD), pdTRUE, (void *)&timer_id, gpio_polling_task);
}

void fad_gpio_begin_polling()
{
    // xTimerStart(s_poll_handle, portMAX_DELAY);
}

// xTaskCreate(gpio_check_task, "GPIO_Check_Task_Handler", TASK_STACK_DEPTH,
//             0, configMAX_PRIORITIES - 5, &s_gpio_check_task_handle);