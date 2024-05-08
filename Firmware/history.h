#ifndef HISTORY_H
#define HISTORY_H

#include "stdint.h"

void history_notify_1h(void);
void history_fill_buffer(uint16_t values_buffer[], uint16_t seconds);

#endif  // HISTORY_H