/**
 * fad_gpio.h
 * 
 * This file is used to interface with the GPIO pins of the ESP32. This includes
 * button handling and aux port continuity checks. In progress.
 * 
 * 
 * Author: Corey Bean
 * Date: 2/18/2021
 * Organization: Collaboratory
 */



/** 
 * @brief Initialize GPIO file for functionality.
 */
void fad_gpio_init();


/**
 * @brief Begin freeRTOS timer and start polling GPIO pins for changes in state.
 */
void fad_gpio_begin_polling();
