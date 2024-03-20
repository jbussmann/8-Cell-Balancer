#ifndef MUX_H
#define MUX_H

#include "stdint.h"

void mux_init(void);
uint32_t mux_get_pwm_task_start(void);

#endif  // MUX_H