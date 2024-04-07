#include "ble_services.h"

#include "bluetooth.h"
#include "nrf_ble_gatt.h"

#define NRF_LOG_MODULE_NAME ble_serv
#include "log.h"
NRF_LOG_MODULE_REGISTER();

#define BLE_BASE_UUID                                                         \
  {                                                                           \
    {                                                                         \
      0xCB, 0xBC, 0xA9, 0x9A, 0x87, 0x78, 0x65, 0x56, 0x43, 0x34, 0x21, 0x12, \
          0x00, 0x00, 0x00, 0x00                                              \
    }                                                                         \
  }
#define BLE_BALANCER_SERVICE_UUID     0xAB00
#define BLE_VOLTAGE_CHAR_UUID  0xAB01
#define BLE_CURRENT_CHAR_UUID  0xAB02
#define BLE_VOLT_DEV_CHAR_UUID 0xAB03
#define BLE_CURR_DEV_CHAR_UUID 0xAB04
#define BLE_PWM_SET_CHAR_UUID  0xAB04

#define BLE_VALUE_CHAR_LENGTH  ((8 * 4) + 7)
#define BLE_PWM_CHAR_LENGTH  ((8 * 3) + 7)

void ble_notify_cell_values(uint32_t values[], uint8_t type) {
  ble_os_t *p_service = ble_get_service();

  if (p_service->connection_handle != BLE_CONN_HANDLE_INVALID) {
    uint16_t len = BLE_VALUE_CHAR_LENGTH;

    char data_sting[BLE_VALUE_CHAR_LENGTH + 1];

    snprintf(data_sting,
             sizeof(data_sting),
             "%4lu,%4lu,%4lu,%4lu,%4lu,%4lu,%4lu,%4lu",
             values[0],
             values[1],
             values[2],
             values[3],
             values[4],
             values[5],
             values[6],
             values[7]);

    ble_gatts_hvx_params_t hvx_params;
    memset(&hvx_params, 0, sizeof(hvx_params));
    switch (type) {
      case VOLTAGE:
        hvx_params.handle = p_service->voltage_handles.value_handle;
        break;
      case CURRENT:
        hvx_params.handle = p_service->current_handles.value_handle;
        break;
      case VOLT_DEV:
        hvx_params.handle = p_service->volt_dev_handles.value_handle;
        break;
      case CURR_DEV:
        hvx_params.handle = p_service->curr_dev_handles.value_handle;
        break;
      case PWM_SET:
        hvx_params.handle = p_service->pwm_set_handles.value_handle;
        break;
    }    
    hvx_params.type = BLE_GATT_HVX_NOTIFICATION;
    hvx_params.offset = 0;
    hvx_params.p_len = &len;
    hvx_params.p_data = (uint8_t *)data_sting;

    sd_ble_gatts_hvx(p_service->connection_handle, &hvx_params);
  }
}

static void ble_voltage_char_add(ble_os_t *p_service) {
  uint32_t err_code;
  ble_uuid_t char_uuid;
  ble_uuid128_t base_uuid = BLE_BASE_UUID;
  char_uuid.uuid = BLE_VOLTAGE_CHAR_UUID;
  err_code = sd_ble_uuid_vs_add(&base_uuid, &char_uuid.type);
  ERROR_CHECK("voltage char uuid add", err_code);

  ble_gatts_attr_md_t client_ccd_metadata;
  memset(&client_ccd_metadata, 0, sizeof(client_ccd_metadata));
  BLE_GAP_CONN_SEC_MODE_SET_OPEN(&client_ccd_metadata.read_perm);
  BLE_GAP_CONN_SEC_MODE_SET_OPEN(&client_ccd_metadata.write_perm);
  client_ccd_metadata.vloc = BLE_GATTS_VLOC_STACK;

  ble_gatts_char_md_t char_metadata;
  memset(&char_metadata, 0, sizeof(char_metadata));
  char_metadata.char_props.read = 1;
  char_metadata.char_props.write = 0;
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
  attr_char_value.max_len = BLE_VALUE_CHAR_LENGTH;
  attr_char_value.init_len = BLE_VALUE_CHAR_LENGTH;
  uint8_t value[BLE_VALUE_CHAR_LENGTH] = {};
  attr_char_value.p_value = value;

  err_code = sd_ble_gatts_characteristic_add(p_service->service_handle,
                                             &char_metadata,
                                             &attr_char_value,
                                             &p_service->voltage_handles);
  ERROR_CHECK("voltage char add", err_code);
}

static void ble_current_char_add(ble_os_t *p_service) {
  uint32_t err_code;
  ble_uuid_t char_uuid;
  ble_uuid128_t base_uuid = BLE_BASE_UUID;
  char_uuid.uuid = BLE_CURRENT_CHAR_UUID;
  err_code = sd_ble_uuid_vs_add(&base_uuid, &char_uuid.type);
  ERROR_CHECK("current char uuid add", err_code);

  ble_gatts_attr_md_t client_ccd_metadata;
  memset(&client_ccd_metadata, 0, sizeof(client_ccd_metadata));
  BLE_GAP_CONN_SEC_MODE_SET_OPEN(&client_ccd_metadata.read_perm);
  BLE_GAP_CONN_SEC_MODE_SET_OPEN(&client_ccd_metadata.write_perm);
  client_ccd_metadata.vloc = BLE_GATTS_VLOC_STACK;

  ble_gatts_char_md_t char_metadata;
  memset(&char_metadata, 0, sizeof(char_metadata));
  char_metadata.char_props.read = 1;
  char_metadata.char_props.write = 0;
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
  attr_char_value.max_len = BLE_VALUE_CHAR_LENGTH;
  attr_char_value.init_len = BLE_VALUE_CHAR_LENGTH;
  uint8_t value[BLE_VALUE_CHAR_LENGTH] = {};
  attr_char_value.p_value = value;

  err_code = sd_ble_gatts_characteristic_add(p_service->service_handle,
                                             &char_metadata,
                                             &attr_char_value,
                                             &p_service->current_handles);
  ERROR_CHECK("current char add", err_code);
}

static void ble_pwm_set_char_add(ble_os_t *p_service) {
  uint32_t err_code;
  ble_uuid_t char_uuid;
  ble_uuid128_t base_uuid = BLE_BASE_UUID;
  char_uuid.uuid = BLE_PWM_SET_CHAR_UUID;
  err_code = sd_ble_uuid_vs_add(&base_uuid, &char_uuid.type);
  ERROR_CHECK("pwm set char uuid add", err_code);

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
  attr_char_value.max_len = BLE_PWM_CHAR_LENGTH;
  attr_char_value.init_len = BLE_PWM_CHAR_LENGTH;
  uint8_t value[BLE_PWM_CHAR_LENGTH] = {};
  attr_char_value.p_value = value;

  err_code = sd_ble_gatts_characteristic_add(p_service->service_handle,
                                             &char_metadata,
                                             &attr_char_value,
                                             &p_service->pwm_set_handles);
  ERROR_CHECK("pwm_set char add", err_code);
}

void ble_service_init(ble_os_t *p_service) {
  uint32_t err_code;
  ble_uuid_t service_uuid;
  ble_uuid128_t base_uuid = BLE_BASE_UUID;
  service_uuid.uuid = BLE_BALANCER_SERVICE_UUID;
  err_code = sd_ble_uuid_vs_add(&base_uuid, &service_uuid.type);
  ERROR_CHECK("balancer service uuid add", err_code);
  err_code = sd_ble_gatts_service_add(
      BLE_GATTS_SRVC_TYPE_PRIMARY, &service_uuid, &p_service->service_handle);
  ERROR_CHECK("balancer service add", err_code);
  ble_voltage_char_add(p_service);
  ble_current_char_add(p_service);
  ble_pwm_set_char_add(p_service);
}