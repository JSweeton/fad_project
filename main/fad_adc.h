
esp_err_t adc_init(void);
esp_err_t adc_buffer_init(void);
esp_err_t adc_timer_init(void);
esp_err_t adc_timer_start(void);
void adc_hdl_evt(uint16_t evt, void *params);
