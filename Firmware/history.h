#ifndef HISTORY_H
#define HISTORY_H

#include "stdint.h"
#include "stdbool.h"

void history_notify_1h_full(void);
void history_notify_12h_full(void);
void history_fill_buffer(uint16_t values_buffer[], uint16_t seconds);

#endif  // HISTORY_H