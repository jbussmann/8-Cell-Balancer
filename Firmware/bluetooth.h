#ifndef BLUETOOTH_H
#define BLUETOOTH_H

#include "nrf_ble_gatt.h"

typedef struct {
  uint16_t connection_handle;
  uint16_t service_handle;
  ble_gatts_char_handles_t char_handles;
} ble_os_t;

void ble_init(void);

#endif  // BLUETOOTH_H
