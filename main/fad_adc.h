


/*
 * Function Prototypes
 */


esp_err_t adc_init(void); //initializes ADC parameters
esp_err_t adc_timer_start(void); //starts ADC timer
void adc_hdl_evt(uint16_t evt, void *params); //event handler for ADC events
