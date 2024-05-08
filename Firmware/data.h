#ifndef DATA_H
#define DATA_H

#include "nrf_saadc.h"

#define NUMBER_OF_CELLS 8

void data_process_buffer(nrf_saadc_value_t *p_buffer);

#endif  // DATA_H
