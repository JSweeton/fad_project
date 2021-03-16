/**
 * fad_gpio.h
 * 
 * This file is used to interface with the GPIO pins of the ESP32. This includes
 * button handling and aux port continuity checks.
 * 
 * 
 * Author: Corey Bean
 * Date: 2/18/2021
 * Organization: Collaboratory
 */



/**
 * @brief [TODO] Check GPIO pins for aux output.
 * @returns 'true' if wired aux output is present, 'false' otherwise
 */
bool fad_gpio_check_wired_output();