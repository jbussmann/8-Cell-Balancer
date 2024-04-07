#ifndef PWM_H
#define PWM_H

#include "nrfx_pwm.h"

void pwm_init(void);
void pwm_start(void);
void pwm_update_values(uint16_t values[]);

#endif  // PWM_H