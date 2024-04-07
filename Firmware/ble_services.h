#ifndef BLE_SERVICES_H
#define BLE_SERVICES_H

#include <stdint.h>

#include "bluetooth.h"

enum { VOLTAGE, CURRENT, VOLT_DEV, CURR_DEV, PWM_SET};

void ble_notify_cell_values(uint32_t values[], uint8_t type);
void ble_service_init(ble_os_t *p_service);

#endif  // BLE_SERVICES_H
