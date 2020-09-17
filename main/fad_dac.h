#include "esp_system.h"



esp_err_t dac_init(void);
void dac_output(uint8_t *dac_buffer, uint16_t dac_buffer_pos);
