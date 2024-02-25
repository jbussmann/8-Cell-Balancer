#ifndef ADC_H
#define ADC_H

#include "stdint.h"

void adc_init(void);
const uint32_t adc_get_task(void);

#endif  // ADC_H
