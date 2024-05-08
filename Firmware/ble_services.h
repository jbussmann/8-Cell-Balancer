#ifndef BLE_SERVICES_H
#define BLE_SERVICES_H

#include <stdint.h>

#include "bluetooth.h"

enum { VALUES, DEVIATIONS, HISTORY_1H, HISTORY_12H, PWM_SET };

void ble_notify_cell_values(uint16_t values[16], uint8_t type);
void ble_notify_history_values(uint16_t values[16], uint8_t type);
void ble_service_init(ble_os_t *p_service);

#endif  // BLE_SERVICES_H
