/*
 * Author: Corey Bean
 * Organization: Messiah Collaboratory
 * Date: 9/17/2020
 * 
 * This file will hold all of the constants for our project. You can change their values directly here
 */

#ifndef _FAD_DEFS_H_
#define _FAD_DEFS_H_

#include <stdlib.h>
#include "esp_system.h"

/* ADC Definitions */
#define ADC_BUFFER_SIZE 2048    //Buffer size for holding ADC data. 
#define MULTISAMPLES 1          //Number of ADC samples per DAC output
#define DAC_BUFFER_SIZE (ADC_BUFFER_SIZE / MULTISAMPLES)  //Buffer size for holding staged DAC data. Hold one DAC sample for each ADC sample divided by multisamples
#define ADC_CHANNEL ADC_CHANNEL_6

/* Timer Definitions */
#define TIMER_FREQ 88200    //Frequency of the Timer
#define ALARM_FREQ 11025    //Determines the frequency of ADC sampling and DAC output
#define OUTPUT_FREQ (ALARM_FREQ / MULTISAMPLES)
#define FAD_GPIO_POLLING_PERIOD 32  // GPIO Check polling period in ms. Check GPIO every period.


/* The GPIO assignments. */
// Should not be between 34-39, as those have no pullup ability
// Buttons
#define FAD_DISC_BN GPIO_NUM_12
#define FAD_VOL_UP_BN GPIO_NUM_13
#define FAD_VOL_DN_BN GPIO_NUM_14

// Wired headset (HS) detection
#define FAD_HS_DETECT_1 GPIO_NUM_32
#define FAD_HS_DETECT_2 GPIO_NUM_33

/* Volume button characteristics. Values correspond to gpio_history values */
#define FAD_VOL_CHANGE_DELAY 10 // Determines how long the button needs to be held before volume begins ramping continuously
#define FAD_VOL_CHANGE_SLOPE 5 // Determines how quickly (period) the button volume ramps 
#define FAD_VOL_STEP 5          // How much the volume jumps after every slope period (as a percentage of max volume)
#define FAD_DISC_ON_DELAY 100 // How long the discovery button needs to be pressed for disc mode to activate

/* The outupt mode determines whether the device is outputting to physical headset or BT */
typedef enum {
    FAD_OUTPUT_DAC,
    FAD_OUTPUT_BT
} fad_output_mode_t;


/* The type of algorithm to be used */
typedef enum {
    FAD_ALGO_DELAY,
    FAD_ALGO_TEMPLATE,
    FAD_ALGO_FREQ_SHIFT,
    FAD_ALGO_PLL,
    FAD_ALGO_MASKING,
} fad_algo_type_t;

/* These modes dictate param choices for each function. Higher modes mean greater algo effects */
typedef enum {
    FAD_ALGO_MODE_1,
    FAD_ALGO_MODE_2,
    FAD_ALGO_MODE_3,
} fad_algo_mode_t;

/* The parameters to be passed to an algorithm initialization function */
typedef union {
    /* FAD_ALGO_DELAY */
    struct algo_delay_params_t {
        int read_size;
        int delay;
    } algo_delay_params;

    /* FAD_ALGO_TEMPLATE */
    struct algo_template_params_t {
        int read_size;
        int period;
    } algo_template_params;

    /* FAD_ALGO_FREQ_SHIFT */
    struct algo_freq_shift_params_t {
        int read_size;      // Number of reads from ADC per algo call
        int shift_amount;   // Shift amount in Hz (based on sampling freq of 11025)
    } algo_freq_shift_params;

    /* FAD_ALGO_MASKING */
    struct algo_masking_params_t {
        int read_size;      // Number of reads from ADC per algo call
    } algo_masking_params;

} fad_algo_init_params_t;


/*
 * Global variables
 */
char *ALGO_TAG;

uint16_t *adc_buffer;
uint8_t *dac_buffer;
uint16_t adc_buffer_pos;
uint16_t dac_buffer_pos;
uint16_t adc_buffer_pos_copy;
uint16_t dac_buffer_pos_copy;

/*
* Function typedefs
*/

/**
 * @brief     algorithm function
 * 
 * @param in_buff Pointer to beginning of ADC buffer (also a global)
 * @param out_buff [OUT] Pointer to beginning of DAC buffer (also a global)
 * @param in_pos Integer index of algorithm beginning location in ADC buffer
 * @param out_pos Integer index of algorithm beginning location in ADC buffer
 * @param multisamples The number of input samples per output
 */
typedef void (* algo_func_t) (uint16_t *in_buff, uint8_t *out_buff, uint16_t in_pos, uint16_t out_pos, int multisamples);

typedef void (* algo_init_func_t) (fad_algo_init_params_t *params);

typedef void (* algo_deinit_func_t) ();

/**
 * @brief     callback function for app events
 */
typedef void (* fad_app_cb_t) (uint16_t event, void *param);


#endif

