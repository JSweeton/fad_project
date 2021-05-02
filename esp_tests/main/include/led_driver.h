/**
 * Description: Drivers to control single RGB LED dimming, colors, flashing. 
 * To use, 'fad_led_update' function must be placed inside timer after the driver has been configured.
 * 
 * Author: Corey Bean
 * 
 * Date: 5/1/2021
 */

#include "driver/gpio.h"


struct fad_led_config_t {
    int timer_freq;     // The frequency with which the update_led function will be invoked
    
};

typedef enum {
    FAD_LED_COLOR_BLUE,   // blue
    FAD_LED_COLOR_RED,
    FAD_LED_COLOR_GREEN,
    FAD_LED_COLOR_YELLOW,
    FAD_LED_COLOR_PURPLE,
    FAD_LED_COLOR_WHITE
} fad_led_color;

/**
 * @brief Place this function in your timer. This calls back to the LED driver
 * and updates the LED's. Must configure timer alarm frequency before placing inside
 * the timer.
 * 
 */ 
void fad_led_update();

void fad_led_set_color(fad_led_color color);

/**
 * @brief Set which GPIO pins connect to which color on the RGB Diode
 * 
 * @param red_pin The gpio num associated with the red pin (i.e. GPIO_NUM_2)
 * @param blue_pin The gpio num associated with the blue pin (i.e. GPIO_NUM_2)
 * @param green_pin The gpio num associated with the green pin (i.e. GPIO_NUM_2)
 * 
 * @returns esp_err_t (ESP_OK if OK, elsewhise if an error occurs)
 */
void fad_led_set_pins(gpio_num_t red_pin, gpio_num_t blue_pin, gpio_num_t green_pin);

void fad_led_init(fad_led_config_t *conf);