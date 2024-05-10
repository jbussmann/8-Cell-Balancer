#include "ble_services.h"

#include "ble_srv_common.h"
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
#define BLE_BALANCER_SERVICE_UUID 0xAB00
#define BLE_VALUE_CHAR_UUID       0xAB01
#define BLE_DEVIATION_CHAR_UUID   0xAB02
#define BLE_HISTORY_1H_CHAR_UUID  0xAB03
#define BLE_HISTORY_12H_CHAR_UUID 0xAB04
#define BLE_PWM_SET_CHAR_UUID     0xAB05

// 2 bytes * (8 voltages + 8 currents)
#define BLE_VALUE_CHAR_LENGTH     (sizeof(uint16_t) * (8 + 8))
// (8 values * 3 characters) + 7 commas
#define BLE_PWM_CHAR_LENGTH       ((8 * 3) + 7)

bool is_notification_enabled(uint8_t type) {
  uint8_t cccd_value[BLE_CCCD_VALUE_LEN];
  ble_os_t *p_service = ble_get_service();
  uint16_t cccd_handle = 0;
  ble_gatts_value_t gatts_val = {
      .len = BLE_CCCD_VALUE_LEN, .offset = 0, .p_value = cccd_value};

  switch (type) {
    case VALUES:
      cccd_handle = p_service->values_handles.cccd_handle;
      break;
    case DEVIATIONS:
      cccd_handle = p_service->deviation_handles.cccd_handle;
      break;
    case HISTORY_1H:
      cccd_handle = p_service->history_1h_handles.cccd_handle;
      break;
    case HISTORY_12H:
      cccd_handle = p_service->history_12h_handles.cccd_handle;
      break;
    default:
      NRF_LOG_ERROR("undefined type")
      break;
  }

  uint32_t err_code = sd_ble_gatts_value_get(
      p_service->connection_handle, cccd_handle, &gatts_val);
  ERROR_CHECK("gatts value get", err_code);

  return ble_srv_is_notification_enabled(cccd_value);
}

void ble_notify_history_values(uint16_t values[], uint8_t type) {
  ble_os_t *p_service = ble_get_service();

  if (p_service->connection_handle == BLE_CONN_HANDLE_INVALID) {
    return;
  }

  if (!is_notification_enabled(type)) {
    return;
  }

  uint16_t len = BLE_HISTORY_CHAR_LENGTH;
  ble_gatts_hvx_params_t hvx_params;

  memset(&hvx_params, 0, sizeof(hvx_params));
  switch (type) {
    case HISTORY_1H:
      hvx_params.handle = p_service->history_1h_handles.value_handle;
      break;
    case HISTORY_12H:
      hvx_params.handle = p_service->history_12h_handles.value_handle;
      break;
  }
  hvx_params.type = BLE_GATT_HVX_NOTIFICATION;
  hvx_params.offset = 0;
  hvx_params.p_len = &len;
  hvx_params.p_data = (uint8_t *)values;

  uint32_t err_code =
      sd_ble_gatts_hvx(p_service->connection_handle, &hvx_params);
  ERROR_CHECK("history char notify", err_code);
}

void ble_notify_cell_values(uint16_t values[], uint8_t type) {
  ble_os_t *p_service = ble_get_service();

  if (p_service->connection_handle == BLE_CONN_HANDLE_INVALID) {
    return;
  }

  if (!is_notification_enabled(type)) {
    return;
  }

  uint16_t len = BLE_VALUE_CHAR_LENGTH;
  ble_gatts_hvx_params_t hvx_params;

  memset(&hvx_params, 0, sizeof(hvx_params));
  switch (type) {
    case VALUES:
      hvx_params.handle = p_service->values_handles.value_handle;
      break;
    case DEVIATIONS:
      hvx_params.handle = p_service->deviation_handles.value_handle;
      break;
  }
  hvx_params.type = BLE_GATT_HVX_NOTIFICATION;
  hvx_params.offset = 0;
  hvx_params.p_len = &len;
  hvx_params.p_data = (uint8_t *)values;

  uint32_t err_code =
      sd_ble_gatts_hvx(p_service->connection_handle, &hvx_params);
  ERROR_CHECK("value char notify", err_code);
}

// could be refactored into a single function
static void ble_values_char_add(ble_os_t *p_service) {
  uint32_t err_code;
  ble_uuid_t char_uuid;
  ble_uuid128_t base_uuid = BLE_BASE_UUID;
  char_uuid.uuid = BLE_VALUE_CHAR_UUID;
  err_code = sd_ble_uuid_vs_add(&base_uuid, &char_uuid.type);
  ERROR_CHECK("value char uuid add", err_code);

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
                                             &p_service->values_handles);
  ERROR_CHECK("value char add", err_code);
}

static void ble_deviation_char_add(ble_os_t *p_service) {
  uint32_t err_code;
  ble_uuid_t char_uuid;
  ble_uuid128_t base_uuid = BLE_BASE_UUID;
  char_uuid.uuid = BLE_DEVIATION_CHAR_UUID;
  err_code = sd_ble_uuid_vs_add(&base_uuid, &char_uuid.type);
  ERROR_CHECK("deviation char uuid add", err_code);

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
                                             &p_service->deviation_handles);
  ERROR_CHECK("deviation char add", err_code);
}

static void ble_history_1h_char_add(ble_os_t *p_service) {
  uint32_t err_code;
  ble_uuid_t char_uuid;
  ble_uuid128_t base_uuid = BLE_BASE_UUID;
  char_uuid.uuid = BLE_HISTORY_1H_CHAR_UUID;
  err_code = sd_ble_uuid_vs_add(&base_uuid, &char_uuid.type);
  ERROR_CHECK("history 1h char uuid add", err_code);

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
  attr_char_value.max_len = BLE_HISTORY_CHAR_LENGTH;
  attr_char_value.init_len = BLE_HISTORY_CHAR_LENGTH;
  uint8_t value[BLE_HISTORY_CHAR_LENGTH] = {};
  attr_char_value.p_value = value;

  err_code = sd_ble_gatts_characteristic_add(p_service->service_handle,
                                             &char_metadata,
                                             &attr_char_value,
                                             &p_service->history_1h_handles);
  ERROR_CHECK("history 1h char add", err_code);
}

static void ble_history_12h_char_add(ble_os_t *p_service) {
  uint32_t err_code;
  ble_uuid_t char_uuid;
  ble_uuid128_t base_uuid = BLE_BASE_UUID;
  char_uuid.uuid = BLE_HISTORY_12H_CHAR_UUID;
  err_code = sd_ble_uuid_vs_add(&base_uuid, &char_uuid.type);
  ERROR_CHECK("history 12h char uuid add", err_code);

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
  attr_char_value.max_len = BLE_HISTORY_CHAR_LENGTH;
  attr_char_value.init_len = BLE_HISTORY_CHAR_LENGTH;
  uint8_t value[BLE_HISTORY_CHAR_LENGTH] = {};
  attr_char_value.p_value = value;

  err_code = sd_ble_gatts_characteristic_add(p_service->service_handle,
                                             &char_metadata,
                                             &attr_char_value,
                                             &p_service->history_12h_handles);
  ERROR_CHECK("history 12h char add", err_code);
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
  char_metadata.char_props.notify = 1;  // unnecessary

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
  ble_values_char_add(p_service);
  ble_deviation_char_add(p_service);
  ble_history_1h_char_add(p_service);
  ble_history_12h_char_add(p_service);
  ble_pwm_set_char_add(p_service);
}