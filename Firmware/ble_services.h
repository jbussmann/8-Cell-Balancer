#ifndef BLE_SERVICES_H
#define BLE_SERVICES_H

#include <stdint.h>

#include "bluetooth.h"

enum { VALUES, DEVIATIONS, HISTORY_12H, HISTORY_1H, PWM_SET };

void ble_notify_cell_values(uint16_t values[16], uint8_t type);
void ble_service_init(ble_os_t *p_service);
void ble_notify_1h_history(void);

#endif  // BLE_SERVICES_H
