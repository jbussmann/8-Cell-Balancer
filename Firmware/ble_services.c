#include "ble_services.h"

#include "app_error.h"
#include "bluetooth.h"
#include "nrf_ble_gatt.h"

#define BLE_UUID_OUR_BASE_UUID                                                \
  {                                                                           \
    {                                                                         \
      0xCB, 0xBC, 0xA9, 0x9A, 0x87, 0x78, 0x65, 0x56, 0x43, 0x34, 0x21, 0x12, \
          0x00, 0x00, 0x00, 0x00                                              \
    }                                                                         \
  }
#define BLE_UUID_OUR_SERVICE            0xAB00
#define BLE_UUID_OUR_CHARACTERISTC_UUID 0xAB01

void ble_characteristic_update(ble_os_t *p_service,
                               int32_t *p_temperature_value) {
  if (p_service->connection_handle != BLE_CONN_HANDLE_INVALID) {
    uint16_t len = 4;

    ble_gatts_hvx_params_t hvx_params;
    memset(&hvx_params, 0, sizeof(hvx_params));
    hvx_params.handle = p_service->char_handles.value_handle;
    hvx_params.type = BLE_GATT_HVX_NOTIFICATION;
    hvx_params.offset = 0;
    hvx_params.p_len = &len;
    hvx_params.p_data = (uint8_t *)p_temperature_value;

    sd_ble_gatts_hvx(p_service->connection_handle, &hvx_params);
  }
}

void ble_characteristic_add(ble_os_t *p_service) {
  uint32_t err_code;
  ble_uuid_t char_uuid;
  ble_uuid128_t base_uuid = BLE_UUID_OUR_BASE_UUID;
  char_uuid.uuid = BLE_UUID_OUR_CHARACTERISTC_UUID;
  err_code = sd_ble_uuid_vs_add(&base_uuid, &char_uuid.type);
  APP_ERROR_CHECK(err_code);

  ble_gatts_attr_md_t client_ccd_metadata;
  memset(&client_ccd_metadata, 0, sizeof(client_ccd_metadata));
  BLE_GAP_CONN_SEC_MODE_SET_OPEN(&client_ccd_metadata.read_perm);
  BLE_GAP_CONN_SEC_MODE_SET_OPEN(&client_ccd_metadata.write_perm);
  client_ccd_metadata.vloc = BLE_GATTS_VLOC_STACK;

  ble_gatts_char_md_t char_metadata;
  memset(&char_metadata, 0, sizeof(char_metadata));
  char_metadata.char_props.read = 1;
  char_metadata.char_props.write = 1;
  char_metadata.p_cccd_md = &client_ccd_metadata;
  char_metadata.char_props.notify = 1;

  ble_gatts_attr_md_t attr_metadata;
  memset(&attr_metadata, 0, sizeof(attr_metadata));
  attr_metadata.vloc = BLE_GATTS_VLOC_STACK;
  BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_metadata.read_perm);
  BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_metadata.write_perm);

  ble_gatts_attr_t attr_char_value;
  memset(&attr_char_value, 0, sizeof(attr_char_value));
  attr_char_value.p_uuid = &char_uuid;
  attr_char_value.p_attr_md = &attr_metadata;
  attr_char_value.max_len = 4;
  attr_char_value.init_len = 4;
  uint8_t value[] = "abcd";
  attr_char_value.p_value = value;

  err_code = sd_ble_gatts_characteristic_add(p_service->service_handle,
                                             &char_metadata,
                                             &attr_char_value,
                                             &p_service->char_handles);
  APP_ERROR_CHECK(err_code);
}

void ble_service_init(ble_os_t *p_service) {
  uint32_t err_code;
  ble_uuid_t service_uuid;
  ble_uuid128_t base_uuid = BLE_UUID_OUR_BASE_UUID;
  service_uuid.uuid = BLE_UUID_OUR_SERVICE;
  err_code = sd_ble_uuid_vs_add(&base_uuid, &service_uuid.type);
  APP_ERROR_CHECK(err_code);
  err_code = sd_ble_gatts_service_add(
      BLE_GATTS_SRVC_TYPE_PRIMARY, &service_uuid, &p_service->service_handle);
  APP_ERROR_CHECK(err_code);
  ble_characteristic_add(p_service);
}