#ifndef _FAD_DAC_H_
#define _FAD_DAC_H_

#include "esp_system.h"


esp_err_t dac_init(void);

void IRAM_ATTR dac_output_value(uint8_t value);

#endif