#ifndef PWM_H
#define PWM_H

#include "nrfx_pwm.h"

void pwm_init(void);
void pwm_start(void);
// void pwm_set_value(nrf_pwm_values_common_t value);

#endif  // PWM_H