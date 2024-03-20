#ifndef LOG_H
#define LOG_H

#include <stdint.h>

#include "bluetooth.h"

void ble_characteristic_update(ble_os_t *p_service,
                               int32_t *p_temperature_value);
void ble_service_init(ble_os_t *p_service);

#endif  // LOG_H
