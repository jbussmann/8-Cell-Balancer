#ifndef PWM_H
#define PWM_H

#include "nrfx_pwm.h"

void pwm_init(void);
void pwm_start(void);
void pwm_update_values(uint16_t values[8]);
void pwm_calculate_next_values(uint16_t voltages[8]);
void pwm_toggle_balancer_state(void);

#endif  // PWM_H