#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "esp_log.h"
#include "driver/gpio.h"

#include "fad_app_core.h"
#include "fad_defs.h"
#include "main.h"

#define BUTTON_BIT_MASK ((1ULL << FAD_VOL_DN_BN) | (1ULL << FAD_VOL_UP_BN) | (1ULL << FAD_DISC_BN))
#define HS_DETECT_BIT_MASK ((1ULL << FAD_HS_DETECT_1) | (1ULL << FAD_HS_DETECT_2))

/* Stores the number of consecutive button hits to keep track of long presses */
typedef struct
{
    int16_t vu; // volume up button
    int16_t vd; // volume down button
    int16_t hs_1; // wired detect pin
    int16_t hs_2; // wired detect pin
    int16_t de; // discovery enable button
} gpio_history;

static TimerHandle_t s_poll_handle;
static const char *GPIO_TAG = "GPIO";
static gpio_history s_gp_hist = {0, 0, 0, 0, 0};

/**
 * @brief Check volume history values to see if a volume change is being requested by button press.
 * @returns Volume change amount, as percentage of max volume. (so +/- FAD_VOL_STEP, or 0)
 */
static int16_t parse_volume_history(int16_t vu, int16_t vd)
{
    if (!vu && !vd)
        return 0;

    // if both buttons have history, then ignore. Reset values to 0.
    else if (vu && vd)
    {
        s_gp_hist.vu = 0;
        s_gp_hist.vd = 0;
        return 0;
    }

    else if (vu == FAD_VOL_CHANGE_DELAY + FAD_VOL_CHANGE_SLOPE)
    {
        s_gp_hist.vu = FAD_VOL_CHANGE_DELAY;    // reset for next change capture
        return FAD_VOL_STEP;
    }

    else if (vd == FAD_VOL_CHANGE_DELAY + FAD_VOL_CHANGE_SLOPE)
    {
        s_gp_hist.vd = FAD_VOL_CHANGE_DELAY;
        return -FAD_VOL_STEP;
    }

    else if (vu == 1 || vd == 1)
    {
        return vu ? FAD_VOL_STEP : -FAD_VOL_STEP;
    }

    return 0;
}

void gpio_polling_task(TimerHandle_t timer)
{
    /* Get pin levels */

    // Buttons
    s_gp_hist.vd = !gpio_get_level(FAD_VOL_DN_BN) ? s_gp_hist.vd + 1 : 0;
    s_gp_hist.vu = !gpio_get_level(FAD_VOL_UP_BN) ? s_gp_hist.vu + 1 : 0;
    s_gp_hist.de = !gpio_get_level(FAD_DISC_BN) ? s_gp_hist.de + 1 : 0;

    s_gp_hist.hs_1 = gpio_get_level(FAD_HS_DETECT_1) ? s_gp_hist.hs_1 + 1 : 0;
    s_gp_hist.hs_2 = gpio_get_level(FAD_HS_DETECT_2) ? s_gp_hist.hs_2 + 1 : 0;


    /* Parse changes in history of these pins */

    int16_t vol_change = parse_volume_history(s_gp_hist.vu, s_gp_hist.vd);
    if (vol_change)
    {
        fad_main_cb_param_t p;
        p.vol_change_info.vol_change = vol_change;
        fad_app_work_dispatch(fad_main_stack_evt_handler, FAD_VOL_CHANGE, &p, sizeof(fad_main_cb_param_t), NULL);
    }
    
    if (s_gp_hist.de == FAD_DISC_ON_DELAY)
    {
        ESP_LOGI(GPIO_TAG, "Discovery activated.");
        fad_app_work_dispatch(fad_main_stack_evt_handler, FAD_DISC_START, NULL, 0, NULL);

    }

    // ESP_LOGI(GPIO_TAG, "HS_1: %d, HS_2: %d", s_gp_hist.hs_1, s_gp_hist.hs_2);

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

    esp_err_t err = gpio_config(&button_config);
    
    if (err)
    {
        ESP_LOGI(GPIO_TAG, "Error: %d", err);
    }

    // Configure Headset detection
    button_config.pin_bit_mask = HS_DETECT_BIT_MASK;
    button_config.pull_down_en = 1;
    button_config.pull_up_en = 0;

    err = gpio_config(&button_config);

    // Initialize button checking timer
    long timer_id;
    s_poll_handle = xTimerCreate("Polling Timer", pdMS_TO_TICKS(FAD_GPIO_POLLING_PERIOD), pdTRUE, (void *)&timer_id, gpio_polling_task);
}

void fad_gpio_begin_polling()
{
    xTimerStart(s_poll_handle, portMAX_DELAY);
}