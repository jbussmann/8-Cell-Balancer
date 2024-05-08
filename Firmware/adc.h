#ifndef ADC_H
#define ADC_H

// max 12bit otherwise danger of type overflow!
#define ADC_RESOLUTON NRF_SAADC_RESOLUTION_12BIT

void adc_init(void);

#endif  // ADC_H
