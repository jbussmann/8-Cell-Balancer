#include "bluetooth.h"

#include "app_timer.h"
#include "ble_advertising.h"
#include "ble_conn_params.h"
#include "ble_gap.h"
#include "ble_services.h"
#include "board.h"
#include "history.h"
#include "nrf_ble_gatt.h"
#include "nrf_ble_qwr.h"
#include "nrf_log_ctrl.h"
#include "nrf_pwr_mgmt.h"
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "pwm.h"

#define NRF_LOG_MODULE_NAME ble
#include "log.h"
NRF_LOG_MODULE_REGISTER();

#define DEVICE_NAME                    "8-Cell Balancer"

#define APP_BLE_OBSERVER_PRIO          3
#define APP_BLE_CONN_CFG_TAG           1

#define HVN_TX_QUEUE_SIZE              20

// in units of 0.625 ms
#define APP_ADV_INTERVAL               64
// in units of seconds
#define APP_ADV_DURATION               BLE_GAP_ADV_TIMEOUT_GENERAL_UNLIMITED

// in units of 0.625 ms
#define FAST_ADVERTISEMENT_INTERVAL    64
#define SLOW_ADVERTISEMENT_INTERVAL    640
// in units of 10 ms
#define FAST_ADVERTISEMENT_DURATION    3000
#define SLOW_ADVERTISEMENT_DURATION    3000

// GAP connection parameters
// Minimum acceptable connection interval (0.5 seconds).
#define MIN_CONN_INTERVAL              MSEC_TO_UNITS(100, UNIT_1_25_MS)
// Maximum acceptable connection interval (1 second).
#define MAX_CONN_INTERVAL              MSEC_TO_UNITS(200, UNIT_1_25_MS)
// Slave latency.
#define SLAVE_LATENCY                  0
// Connection supervisory time-out (4 seconds).
#define CONN_SUP_TIMEOUT               MSEC_TO_UNITS(4000, UNIT_10_MS)

// Time from initiating event (connect or start of notification) to first time
// sd_ble_gap_conn_param_update is called (15 seconds).
#define FIRST_CONN_PARAMS_UPDATE_DELAY APP_TIMER_TICKS(20000)
// Time between each call to sd_ble_gap_conn_param_update after the first call
// (5 seconds).
#define NEXT_CONN_PARAMS_UPDATE_DELAY  APP_TIMER_TICKS(5000)
// Number of attempts before giving up the connection parameter negotiation.
#define MAX_CONN_PARAMS_UPDATE_COUNT   3

NRF_BLE_GATT_DEF(gatt_instance);
NRF_BLE_QWR_DEF(qwr_instance);
BLE_ADVERTISING_DEF(advertising_instance);

static ble_os_t service = {.connection_handle = BLE_CONN_HANDLE_INVALID};

ble_os_t *ble_get_service(void) { return &service; }
static void timers_init(void) {
  ret_code_t err_code = app_timer_init();
  ERROR_CHECK("app timer init", err_code);
}

static void gap_init(void) {
  ret_code_t err_code;
  ble_gap_conn_params_t gap_conn_params;
  ble_gap_conn_sec_mode_t sec_mode;

  BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);

  err_code = sd_ble_gap_device_name_set(
      &sec_mode, (const uint8_t *)DEVICE_NAME, strlen(DEVICE_NAME));
  ERROR_CHECK("connection parameter init", err_code);

  memset(&gap_conn_params, 0, sizeof(gap_conn_params));

  gap_conn_params.min_conn_interval = MIN_CONN_INTERVAL;
  gap_conn_params.max_conn_interval = MAX_CONN_INTERVAL;
  gap_conn_params.slave_latency = SLAVE_LATENCY;
  gap_conn_params.conn_sup_timeout = CONN_SUP_TIMEOUT;

  err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
  ERROR_CHECK("gap ppcp set", err_code);
}

static void gatt_init(void) {
  ret_code_t err_code = nrf_ble_gatt_init(&gatt_instance, NULL);
  ERROR_CHECK("gatt init", err_code);
}

static void advertising_start(void) {
  ret_code_t err_code =
      ble_advertising_start(&advertising_instance, BLE_ADV_MODE_FAST);
  ERROR_CHECK("advertising start", err_code);
}

static void ble_adverting_handler(ble_adv_evt_t ble_adv_evt) {
  nrf_gpio_pin_clear(ADVERTISE_LED);  // on
  switch (ble_adv_evt) {
    case BLE_ADV_EVT_IDLE:
      NRF_LOG_INFO("Advertising event: Idle");
      nrf_gpio_pin_set(ADVERTISE_LED);  // off
      advertising_start();
      break;
    case BLE_ADV_EVT_DIRECTED_HIGH_DUTY:
      NRF_LOG_INFO("Advertising event: Directed HD");
      break;
    case BLE_ADV_EVT_DIRECTED:
      NRF_LOG_INFO("Advertising event: Directed");
      break;
    case BLE_ADV_EVT_FAST:
      NRF_LOG_INFO("Advertising event: Fast");
      break;
    case BLE_ADV_EVT_SLOW:
      NRF_LOG_INFO("Advertising event: Slow");
      break;
    default:
      NRF_LOG_WARNING("Advertising event: 0x%x (%i)", ble_adv_evt, ble_adv_evt);
      break;
  }
}

static void advertising_init(void) {
  ret_code_t err_code;
  ble_advertising_init_t init;

  memset(&init, 0, sizeof(init));
  init.advdata.name_type = BLE_ADVDATA_FULL_NAME;
  init.advdata.include_appearance = true;
  init.advdata.flags = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;
  init.config.ble_adv_fast_enabled = true;
  init.config.ble_adv_slow_enabled = false;
  init.config.ble_adv_fast_interval = FAST_ADVERTISEMENT_INTERVAL;
  init.config.ble_adv_fast_timeout = FAST_ADVERTISEMENT_DURATION;
  init.config.ble_adv_slow_interval = SLOW_ADVERTISEMENT_INTERVAL;
  init.config.ble_adv_slow_timeout = SLOW_ADVERTISEMENT_DURATION;
  init.evt_handler = ble_adverting_handler;
  init.error_handler = NULL;

  err_code = ble_advertising_init(&advertising_instance, &init);
  ERROR_CHECK("advertising init", err_code);
  ble_advertising_conn_cfg_tag_set(&advertising_instance, APP_BLE_CONN_CFG_TAG);
}

static void nrf_qwr_error_handler(uint32_t nrf_error) {
  NRF_LOG_ERROR("QWR error %s", nrf_strerror_get(nrf_error));
}

static void qwr_init(void) {
  ret_code_t err_code;
  nrf_ble_qwr_init_t qwr_init = {0};

  qwr_init.error_handler = nrf_qwr_error_handler;

  err_code = nrf_ble_qwr_init(&qwr_instance, &qwr_init);
  ERROR_CHECK("qwr init", err_code);
}

static void connection_params_event_handler(ble_conn_params_evt_t *p_evt) {
  uint16_t event_type = p_evt->evt_type;
  switch (event_type) {
    case BLE_CONN_PARAMS_EVT_FAILED:
      NRF_LOG_DEBUG("Connection parameters event: Failed");
      break;
    case BLE_CONN_PARAMS_EVT_SUCCEEDED:
      NRF_LOG_DEBUG("Connection parameters event: Succeeded");
      break;
    default:
      NRF_LOG_WARNING("Connection parameters event: 0x%x", event_type);
      break;
  }
}

static void connection_params_error_handler(uint32_t nrf_error) {
  NRF_LOG_ERROR("Connection parameters error %s", nrf_strerror_get(nrf_error));
}

static void connection_init(void) {
  ret_code_t err_code;
  ble_conn_params_init_t cp_init;

  memset(&cp_init, 0, sizeof(cp_init));

  cp_init.p_conn_params = NULL;
  cp_init.first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY;
  cp_init.next_conn_params_update_delay = NEXT_CONN_PARAMS_UPDATE_DELAY;
  cp_init.max_conn_params_update_count = MAX_CONN_PARAMS_UPDATE_COUNT;
  cp_init.start_on_notify_cccd_handle = BLE_GATT_HANDLE_INVALID;
  cp_init.disconnect_on_fail = true;
  cp_init.evt_handler = connection_params_event_handler;
  cp_init.error_handler = connection_params_error_handler;

  err_code = ble_conn_params_init(&cp_init);
  ERROR_CHECK("connection parameter init", err_code);
}

static void on_gatts_event_write(ble_evt_t const *p_ble_evt) {
  ble_gatts_evt_write_t const *p_evt = &p_ble_evt->evt.gatts_evt.params.write;
  uint16_t attr_handle = p_evt->handle;

  if (attr_handle == service.values_handles.cccd_handle) {
    NRF_LOG_INFO("values characteristic notify %s",
                 p_evt->data[0] ? "enabled" : "disabled");
  } else if (attr_handle == service.deviation_handles.cccd_handle) {
    NRF_LOG_INFO("deviation characteristic notify %s",
                 p_evt->data[0] ? "enabled" : "disabled");
  } else if (attr_handle == service.history_1h_handles.cccd_handle) {
    NRF_LOG_INFO("history 1h characteristic notify %s",
                 p_evt->data[0] ? "enabled" : "disabled");
    if (p_evt->data[0]) {
      history_notify_1h(true);
    }
  } else if (attr_handle == service.history_12h_handles.cccd_handle) {
    NRF_LOG_INFO("history 12h characteristic notify %s",
                 p_evt->data[0] ? "enabled" : "disabled");
  } else if (attr_handle == service.pwm_set_handles.value_handle) {
    NRF_LOG_INFO("pwm characteristic written");
    uint8_t const *p_data = p_evt->data;
    uint16_t values[8];
    for (size_t i = 0; i < 8; i++) {
      char str[3 + 1] = {'\0'};
      strncpy(str, (char *)p_data + (i * 4), 3);
      values[i] = atoi(str);
    }
    static char value_string[51] = {};  // static for logger
    snprintf(value_string,
             sizeof(value_string),
             "pwm values: %i, %i, %i, %i, %i, %i, %i, %i",
             values[0],
             values[1],
             values[2],
             values[3],
             values[4],
             values[5],
             values[6],
             values[7]);
    NRF_LOG_INFO("%s", value_string);
    pwm_update_values(values);
  } else {
    NRF_LOG_WARNING("Unmapped attribute written %i", attr_handle);
  }
}

static void ble_event_handler(ble_evt_t const *p_ble_evt, void *p_context) {
  ret_code_t err_code;

  uint16_t event_id = p_ble_evt->header.evt_id;
  switch (event_id) {
    case BLE_GAP_EVT_CONNECTED:
      NRF_LOG_INFO("Stack event: Connected");
      nrf_gpio_pin_clear(CONNECTED_LED);
      nrf_gpio_pin_set(ADVERTISE_LED);
      service.connection_handle = p_ble_evt->evt.gap_evt.conn_handle;
      err_code = nrf_ble_qwr_conn_handle_assign(&qwr_instance,
                                                service.connection_handle);
      ERROR_CHECK("qwr connection handle assign", err_code);
      err_code =
          sd_ble_gatts_sys_attr_set(service.connection_handle, NULL, 0, 0);
      ERROR_CHECK("system attribute set", err_code);
      break;
    case BLE_GAP_EVT_DISCONNECTED:
      NRF_LOG_INFO("Stack event: Disconnected");
      nrf_gpio_pin_set(CONNECTED_LED);
      service.connection_handle = BLE_CONN_HANDLE_INVALID;
      // advertising_start();
      break;
    case BLE_GAP_EVT_CONN_PARAM_UPDATE:
      NRF_LOG_INFO("Stack event: Connection parameters updated");
      break;
    case BLE_GAP_EVT_PHY_UPDATE_REQUEST:
      NRF_LOG_DEBUG("Stack event: PHY update request");
      ble_gap_phys_t const phys = {
          .rx_phys = BLE_GAP_PHY_AUTO,
          .tx_phys = BLE_GAP_PHY_AUTO,
      };
      err_code =
          sd_ble_gap_phy_update(p_ble_evt->evt.gap_evt.conn_handle, &phys);
      ERROR_CHECK("gap phy update", err_code);
      break;
    case BLE_GAP_EVT_ADV_SET_TERMINATED:
      NRF_LOG_DEBUG("Stack event: Advertising terminated");
      break;
    case BLE_GATTS_EVT_WRITE:
      on_gatts_event_write(p_ble_evt);
      break;
    case BLE_GATTS_EVT_SYS_ATTR_MISSING:
      NRF_LOG_DEBUG("Stack event: SYS attr missing");
      break;
    case BLE_GATTS_EVT_EXCHANGE_MTU_REQUEST:
      NRF_LOG_DEBUG("Stack event: Exchange MTU request");
      break;
    case BLE_GATTS_EVT_HVN_TX_COMPLETE:
      NRF_LOG_DEBUG("Stack event: Notification transmission complete");
      break;
    default:
      NRF_LOG_WARNING("Stack event: Unmapped ID 0x%x (%i)", event_id, event_id);
      break;
  }
}

static void soft_device_init(void) {
  ret_code_t err_code;

  err_code = nrf_sdh_enable_request();
  // ERROR_CHECK("sdh enable request", err_code);

  // reset device because BMP is unable to reset core after loading firmware
  if (err_code == NRF_ERROR_INVALID_STATE) {
    NRF_LOG_ERROR("SoftDevice enable request failed, resetting...");
    NRF_LOG_FINAL_FLUSH();
    NVIC_SystemReset();
  }

  // Configure the BLE stack using the default settings.
  // Fetch the start address of the application RAM.
  uint32_t ram_start = 0;
  err_code = nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start);
  ERROR_CHECK("sdh ble default config set", err_code);

  // configure HVN tx buffer size
  ble_cfg_t ble_cfg;
  memset(&ble_cfg, 0, sizeof(ble_cfg));
  ble_cfg.conn_cfg.conn_cfg_tag = APP_BLE_CONN_CFG_TAG;
  ble_cfg.conn_cfg.params.gatts_conn_cfg.hvn_tx_queue_size = HVN_TX_QUEUE_SIZE;
  err_code = sd_ble_cfg_set(BLE_CONN_CFG_GATTS, &ble_cfg, ram_start);
  ERROR_CHECK("sdh ble custom config set", err_code);

  // Enable BLE stack.
  err_code = nrf_sdh_ble_enable(&ram_start);
  ERROR_CHECK("sdh ble enable (enable sdh_ble logs)", err_code);

  // Register a handler for BLE events.
  NRF_SDH_BLE_OBSERVER(
      stack_observer, APP_BLE_OBSERVER_PRIO, ble_event_handler, NULL);
  // NRF_SDH_BLE_OBSERVER(
  //     advertising_observer, APP_BLE_OBSERVER_PRIO,
  //     ble_advertising_on_ble_evt, NULL);
}

static void power_management_init(void) {
  ret_code_t err_code;
  err_code = nrf_pwr_mgmt_init();
  ERROR_CHECK("power management init", err_code);
}

void ble_init(void) {
  timers_init();
  power_management_init();
  soft_device_init();
  gap_init();
  gatt_init();
  qwr_init();
  ble_service_init(&service);
  advertising_init();
  connection_init();
  advertising_start();
}
