#ifndef BLUETOOTH_H
#define BLUETOOTH_H

#include "nrf_ble_gatt.h"

typedef struct {
  uint16_t connection_handle;
  uint16_t service_handle;
  ble_gatts_char_handles_t values_handles;
  ble_gatts_char_handles_t deviation_handles;
  ble_gatts_char_handles_t history_1h_handles;
  ble_gatts_char_handles_t history_12h_handles;
  ble_gatts_char_handles_t pwm_set_handles;
} ble_os_t;

void ble_init(void);
ble_os_t* ble_get_service(void);

#endif  // BLUETOOTH_H
