#include "adc.h"

#include "ble_services.h"
#include "board.h"
#include "mux.h"
#include "nrf_saadc.h"
#include "nrfx_ppi.h"
#include "nrfx_saadc.h"
#include "nrfx_timer.h"
#include "pwm.h"
#include "sdk_config.h"

#define NRF_LOG_MODULE_NAME adc
#include "log.h"
NRF_LOG_MODULE_REGISTER();

#define SAADC_CHANNEL_CONF(_pin_p, _index)                                 \
  {                                                                        \
    .channel_config =                                                      \
        {                                                                  \
            .resistor_p = NRF_SAADC_RESISTOR_DISABLED,                     \
            .resistor_n = NRF_SAADC_RESISTOR_DISABLED,                     \
            .gain = NRF_SAADC_GAIN1,                                       \
            .reference = NRF_SAADC_REFERENCE_VDD4,                         \
            .acq_time = NRF_SAADC_ACQTIME_3US,                             \
            .mode = NRF_SAADC_MODE_SINGLE_ENDED,                           \
            .burst = NRF_SAADC_BURST_DISABLED,                             \
        },                                                                 \
    .pin_p = (nrf_saadc_input_t)_pin_p, .pin_n = NRF_SAADC_INPUT_DISABLED, \
    .channel_index = _index,                                               \
  }

static const nrfx_saadc_channel_t channels_config[] = {
    SAADC_CHANNEL_CONF(LOWER_MUX_ADC, 0),
    SAADC_CHANNEL_CONF(UPPER_MUX_ADC, 1),
};
#define ADC_CHANNEL_COUNT      NRFX_ARRAY_SIZE(channels_config)

#define ADC_SAMPLE_START_TICKS 5
#define ADC_CLEAR_TIMER_TICKS  25

#define ADC_NUMBER_OF_SAMPLES  128  // 8x 16 Signals

// max 12bit otherwise danger of type overflow!
#define ADC_RESOLUTON          NRF_SAADC_RESOLUTION_12BIT
#define ADC_RESOLUTON_BITS     (8 + (2 * ADC_RESOLUTON))
#define ADC_RANGE              (1 << ADC_RESOLUTON_BITS)

static nrf_saadc_value_t samples_buffer[ADC_NUMBER_OF_SAMPLES];

#define NUMBER_OF_CELLS   8
#define NUMBER_OF_SAMPLES 8  // danger of type overflow

typedef struct {
  nrf_saadc_value_t raw_values[NUMBER_OF_SAMPLES];
  // in millivolt/milliampere
  uint16_t avg_value_millis;
  uint16_t deviation_millis;
} cell_values_t;

typedef struct {
  cell_values_t voltage;
  cell_values_t current;
} cell_t;

typedef struct {
  uint32_t voltage[NUMBER_OF_CELLS];
  uint32_t current[NUMBER_OF_CELLS];
  uint32_t volt_dev[NUMBER_OF_CELLS];
  uint32_t curr_dev[NUMBER_OF_CELLS];
  uint16_t length;
} ble_values_t;

static ble_values_t ble_values;

#define HISTORY_BUFFER_ELEMENTS  120
#define HISTORY_1H_INTERVALL     30
#define HISTORY_12H_INTERVALL    360
#define HISTORY_12H_1H_INTERVALL (HISTORY_12H_INTERVALL / HISTORY_1H_INTERVALL)

static uint16_t history_2min_buffer[HISTORY_BUFFER_ELEMENTS][16] = {
    [0 ... HISTORY_BUFFER_ELEMENTS - 1] = {[0 ... 15] = 0xffff}};
static uint16_t history_1h_buffer[HISTORY_BUFFER_ELEMENTS][16] = {
    [0 ... HISTORY_BUFFER_ELEMENTS - 1] = {[0 ... 15] = 0xffff}};
static uint16_t history_12h_buffer[HISTORY_BUFFER_ELEMENTS][16] = {
    [0 ... HISTORY_BUFFER_ELEMENTS - 1] = {[0 ... 15] = 0xffff}};

// position of most recent data point
static uint8_t history_2min_head = HISTORY_BUFFER_ELEMENTS - 1;
static uint8_t history_1h_head = HISTORY_BUFFER_ELEMENTS - 1;
static uint8_t history_12h_head = HISTORY_BUFFER_ELEMENTS - 1;

static void adc_deinterlace_buffer(cell_t cells[],
                                   nrf_saadc_value_t *p_buffer) {
  nrf_saadc_value_t *p_current_samples;

  const uint8_t voltage_sequence[] = {0, 2, 4, 6, 9, 13, 11, 15};
  const uint8_t current_sequence[] = {8, 12, 10, 14, 1, 3, 5, 7};

  for (size_t i = 0; i < NUMBER_OF_SAMPLES; i++) {
    p_current_samples = &p_buffer[i * NUMBER_OF_CELLS * 2];
    for (size_t k = 0; k < NUMBER_OF_CELLS; k++) {
      cells[k].voltage.raw_values[i] = p_current_samples[voltage_sequence[k]];
      cells[k].current.raw_values[i] = p_current_samples[current_sequence[k]];
    }
  }
}

static uint16_t adc_raw_to_voltage(uint16_t raw) {
  uint32_t val = raw * 825 * 267 / (47 * ADC_RANGE);
  return (uint16_t)val;
}

static uint16_t adc_raw_to_current(uint16_t raw) {
  uint32_t val = raw * 825 * 62 / (47 * ADC_RANGE);
  return (uint16_t)val;
}

static void adc_aggregate_voltage(cell_t cells[]) {
  for (size_t i = 0; i < NUMBER_OF_CELLS; i++) {
    int16_t temp_min = ADC_RANGE;
    int16_t temp_max = 0;
    int32_t temp_sum = 0;
    for (size_t k = 0; k < NUMBER_OF_SAMPLES; k++) {
      int16_t val = cells[i].voltage.raw_values[k];
      temp_sum += val;
      if (temp_max < val) {
        temp_max = val;
      } else if (val < temp_min) {
        temp_min = val;
      }
    }
    temp_sum = temp_sum < 0 ? 0 : temp_sum;
    cells[i].voltage.avg_value_millis =
        adc_raw_to_voltage(temp_sum / NUMBER_OF_SAMPLES);
    cells[i].voltage.deviation_millis = adc_raw_to_voltage(temp_max - temp_min);
  }
}

static void adc_aggregate_current(cell_t cells[]) {
  for (size_t i = 0; i < NUMBER_OF_CELLS; i++) {
    int16_t temp_min = ADC_RANGE;
    int16_t temp_max = 0;
    int32_t temp_sum = 0;
    for (size_t k = 0; k < NUMBER_OF_SAMPLES; k++) {
      int16_t val = cells[i].current.raw_values[k];
      temp_sum += val;
      if (temp_max < val) {
        temp_max = val;
      } else if (val < temp_min) {
        temp_min = val;
      }
    }
    temp_sum = temp_sum < 0 ? 0 : temp_sum;
    cells[i].current.avg_value_millis =
        adc_raw_to_current(temp_sum / NUMBER_OF_SAMPLES);
    cells[i].current.deviation_millis = adc_raw_to_current(temp_max - temp_min);
  }
}

static void adc_add_values_to_ble_struct(cell_t cells[]) {
  for (size_t i = 0; i < NUMBER_OF_CELLS; i++) {
    ble_values.voltage[i] += cells[i].voltage.avg_value_millis;
    ble_values.current[i] += cells[i].current.avg_value_millis;

    if (ble_values.volt_dev[i] < cells[i].voltage.deviation_millis) {
      ble_values.volt_dev[i] = cells[i].voltage.deviation_millis;
    }

    if (ble_values.curr_dev[i] < cells[i].current.deviation_millis) {
      ble_values.curr_dev[i] = cells[i].current.deviation_millis;
    }
  }
  ble_values.length += 1;
}

static void adc_prepare_ble_transmission() {
  for (size_t i = 0; i < NUMBER_OF_CELLS; i++) {
    ble_values.voltage[i] /= ble_values.length;
    ble_values.current[i] /= ble_values.length;
  }
}

static void adc_log_values() {
  static char voltage_string[47] = {};  // static for logger
  snprintf(voltage_string,
           sizeof(voltage_string),
           "%4lu,%4lu,%4lu,%4lu,%4lu,%4lu,%4lu,%4lu",
           ble_values.voltage[0],
           ble_values.voltage[1],
           ble_values.voltage[2],
           ble_values.voltage[3],
           ble_values.voltage[4],
           ble_values.voltage[5],
           ble_values.voltage[6],
           ble_values.voltage[7]);
  NRF_LOG_INFO("%s", voltage_string);

  static char current_string[47] = {};  // static for logger
  snprintf(current_string,
           sizeof(current_string),
           "%4lu,%4lu,%4lu,%4lu,%4lu,%4lu,%4lu,%4lu",
           ble_values.current[0],
           ble_values.current[1],
           ble_values.current[2],
           ble_values.current[3],
           ble_values.current[4],
           ble_values.current[5],
           ble_values.current[6],
           ble_values.current[7]);
  NRF_LOG_INFO("%s", current_string);
}

static void adc_fill_history(uint16_t values_buffer[2 * NUMBER_OF_CELLS],
                             uint16_t seconds) {
  history_2min_head++;
  history_2min_head %= HISTORY_BUFFER_ELEMENTS;
  uint16_t *p_head = &history_2min_buffer[history_2min_head][0];
  for (size_t i = 0; i < 2 * NUMBER_OF_CELLS; i++) {
    *(p_head + i) = values_buffer[i];
  }
  NRF_LOG_INFO("%i: 2min buffer added @ pos%i", seconds, history_2min_head);

  if (seconds % HISTORY_1H_INTERVALL == 0) {
    history_1h_head++;
    history_1h_head %= HISTORY_BUFFER_ELEMENTS;
    for (size_t i = 0; i < NUMBER_OF_CELLS; i++) {
      uint32_t sum_v = 0;
      uint32_t sum_i = 0;
      for (size_t k = 0; k < HISTORY_1H_INTERVALL; k++) {
        sum_v += history_2min_buffer[history_2min_head - k][i];
        sum_i += history_2min_buffer[history_2min_head - k][i + 1];
      }
      history_1h_buffer[history_1h_head][i] = sum_v / HISTORY_1H_INTERVALL;
      history_1h_buffer[history_1h_head][i + 1] = sum_i / HISTORY_1H_INTERVALL;
    }
    NRF_LOG_INFO("%i: 1h buffer added @ pos%i", seconds, history_1h_head);
  }

  if (seconds % HISTORY_12H_INTERVALL == 0) {
    history_12h_head++;
    history_12h_head %= HISTORY_BUFFER_ELEMENTS;
    for (size_t i = 0; i < NUMBER_OF_CELLS; i++) {
      uint32_t sum_v = 0;
      uint32_t sum_i = 0;
      for (size_t k = 0; k < HISTORY_12H_1H_INTERVALL; k++) {
        sum_v += history_1h_buffer[history_1h_head - k][i];
        sum_i += history_1h_buffer[history_1h_head - k][i + 1];
      }
      history_12h_buffer[history_12h_head][i] =
          sum_v / HISTORY_12H_1H_INTERVALL;
      history_12h_buffer[history_12h_head][i + 1] =
          sum_i / HISTORY_12H_1H_INTERVALL;
    }
    NRF_LOG_INFO("%i: 12h buffer added @ pos%i", seconds, history_12h_head);
  }
}

static void adc_process_buffer(nrfx_saadc_done_evt_t done_event) {
  cell_t cells[NUMBER_OF_CELLS];
  static uint16_t seconds_counter = 0;

  adc_deinterlace_buffer(cells, done_event.p_buffer);
  adc_aggregate_voltage(cells);
  adc_aggregate_current(cells);

  uint16_t cell_voltages[NUMBER_OF_CELLS];
  for (size_t i = 0; i < NUMBER_OF_CELLS; i++) {
    cell_voltages[i] = cells[i].voltage.avg_value_millis;
  }
  pwm_calculate_next_values(cell_voltages);

  adc_add_values_to_ble_struct(cells);

  if (1000 <= ble_values.length) {
    seconds_counter++;  // overflow is not handled!!

    //       for i in range(8):
    //     mean = sum(self.values["voltage_2min"][i][-nums:])/nums
    //     self.values["voltage_1h"][i].pop(0)
    //     self.values["voltage_1h"][i].append(mean)
    //     mean = sum(self.values["current_2min"][i][-nums:])/nums
    //     self.values["current_1h"][i].pop(0)
    //     self.values["current_1h"][i].append(mean)
    // print(f"{self.counter_seconds}s: {nums}nums 2min->1h")

    adc_prepare_ble_transmission();

    static uint16_t values_buffer[(2 * NUMBER_OF_CELLS)] = {0};
    static uint16_t deviations_buffer[(2 * NUMBER_OF_CELLS)] = {0};

    adc_log_values();

    for (size_t i = 0; i < NUMBER_OF_CELLS; i++) {
      values_buffer[(2 * i)] = (uint16_t)ble_values.voltage[i];
      values_buffer[(2 * i) + 1] = (uint16_t)ble_values.current[i];
      deviations_buffer[(2 * i)] = (uint16_t)ble_values.volt_dev[i];
      deviations_buffer[(2 * i) + 1] = (uint16_t)ble_values.curr_dev[i];
    }

    adc_fill_history(values_buffer, seconds_counter);

    ble_notify_cell_values(values_buffer, VALUES);
    ble_notify_cell_values(deviations_buffer, DEVIATIONS);
    memset(&ble_values, 0, sizeof(ble_values));
  }
}

static void saadc_handler(nrfx_saadc_evt_t const *p_event) {
  switch (p_event->type) {
    case NRFX_SAADC_EVT_DONE:  // result of EVT_END, current buffer is filled
      // NRF_LOG_DEBUG("SAADC-DONE event");
      // NRF_SAADC_STATE_ADV_MODE_SAMPLE_STARTED
      adc_process_buffer(p_event->data.done);

      // reinitialize adc to prevent sample swaps by clearing excess samples
      nrfx_saadc_uninit();
      adc_init();

      mux_pwm_start();
      break;
    case NRFX_SAADC_EVT_LIMIT:
      NRF_LOG_DEBUG("SAADC-LIMIT event");
      break;
    case NRFX_SAADC_EVT_CALIBRATEDONE:
      NRF_LOG_DEBUG("SAADC-CALIBRATEDONE event");
      break;
    case NRFX_SAADC_EVT_BUF_REQ:  // result of EVT_STARTED
      // NRF_LOG_DEBUG("SAADC-BUF_REQ event");
      break;
    case NRFX_SAADC_EVT_READY:  // result of EVT_STARTED
      // NRF_LOG_DEBUG("SAADC-READY event");
      break;
    case NRFX_SAADC_EVT_FINISHED:  // result of EVT_END, all buffers are filled
      // NRF_LOG_DEBUG("SAADC-FINISHED event");  // NRF_SAADC_STATE_ADV_MODE
      break;
    default:
      NRF_LOG_WARNING("SAADC unmapped event");
      break;
  }
}

void adc_init(void) {
  nrfx_err_t status;

  status = nrfx_saadc_init(NRFX_SAADC_CONFIG_IRQ_PRIORITY);
  ERROR_CHECK("SAADC init", status);  // NRF_SAADC_STATE_IDLE

  status = nrfx_saadc_channels_config(channels_config, ADC_CHANNEL_COUNT);
  ERROR_CHECK("SAADC config", status);

  status = nrfx_saadc_offset_calibrate(NULL);     // NULL -> blocking
  ERROR_CHECK("SAADC offset calibrate", status);  // NRF_SAADC_STATE_IDLE

  nrfx_saadc_adv_config_t adv_config = {
      .oversampling = NRF_SAADC_OVERSAMPLE_DISABLED,
      .burst = NRF_SAADC_BURST_DISABLED,
      .internal_timer_cc = 0,
      .start_on_end = false};

  status = nrfx_saadc_advanced_mode_set(
      0b00000011, ADC_RESOLUTON, &adv_config, saadc_handler);
  ERROR_CHECK("SAADC mode set", status);
  // NRF_SAADC_STATE_ADV_MODE, both buffers set to NULL

  status = nrfx_saadc_buffer_set(samples_buffer, ADC_NUMBER_OF_SAMPLES);
  ERROR_CHECK("SAADC samples buffer set", status);

  status = nrfx_saadc_mode_trigger();
  ERROR_CHECK("SAADC trigger", status);
  // NRF_SAADC_STATE_ADV_MODE_SAMPLE ->
  // NRF_SAADC_STATE_ADV_MODE_SAMPLE_STARTED
}
