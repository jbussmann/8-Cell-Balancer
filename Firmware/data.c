#include "data.h"

#include "adc.h"
#include "ble_services.h"
#include "history.h"
#include "pwm.h"

#define NRF_LOG_MODULE_NAME data
#include "log.h"
NRF_LOG_MODULE_REGISTER();

#define ADC_RESOLUTON_BITS (8 + (2 * ADC_RESOLUTON))  // nrf enum magic numbers
#define ADC_RANGE          (1 << ADC_RESOLUTON_BITS)

#define NUMBER_OF_SAMPLES  8  // danger of type overflow

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

static void data_deinterlace_buffer(cell_t cells[],
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

static uint16_t data_raw_to_voltage(uint16_t raw) {
  uint32_t val = raw * 825 * 267 / (47 * ADC_RANGE);
  return (uint16_t)val;
}

static uint16_t data_raw_to_current(uint16_t raw) {
  uint32_t val = raw * 825 * 62 / (47 * ADC_RANGE);
  return (uint16_t)val;
}

static void data_aggregate_voltage(cell_t cells[]) {
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
        data_raw_to_voltage(temp_sum / NUMBER_OF_SAMPLES);
    cells[i].voltage.deviation_millis =
        data_raw_to_voltage(temp_max - temp_min);
  }
}

static void data_aggregate_current(cell_t cells[]) {
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
        data_raw_to_current(temp_sum / NUMBER_OF_SAMPLES);
    cells[i].current.deviation_millis =
        data_raw_to_current(temp_max - temp_min);
  }
}

static void data_add_values_to_ble_struct(cell_t cells[]) {
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

static void data_prepare_ble_transmission() {
  for (size_t i = 0; i < NUMBER_OF_CELLS; i++) {
    ble_values.voltage[i] /= ble_values.length;
    ble_values.current[i] /= ble_values.length;
  }
}

static void data_log_values() {
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

void data_process_buffer(nrf_saadc_value_t *p_buffer) {
  cell_t cells[NUMBER_OF_CELLS];
  static uint16_t seconds_counter = 0;

  data_deinterlace_buffer(cells, p_buffer);
  data_aggregate_voltage(cells);
  data_aggregate_current(cells);

  uint16_t cell_voltages[NUMBER_OF_CELLS];
  for (size_t i = 0; i < NUMBER_OF_CELLS; i++) {
    cell_voltages[i] = cells[i].voltage.avg_value_millis;
  }
  pwm_calculate_next_values(cell_voltages);

  data_add_values_to_ble_struct(cells);

  if (1000 <= ble_values.length) {
    seconds_counter++;  // overflow is not handled!!

    data_prepare_ble_transmission();

    static uint16_t val_buffer[(sizeof(uint16_t) * NUMBER_OF_CELLS)] = {0};
    static uint16_t dev_buffer[(sizeof(uint16_t) * NUMBER_OF_CELLS)] = {0};

    data_log_values();

    for (size_t i = 0; i < NUMBER_OF_CELLS; i++) {
      val_buffer[(2 * i)] = (uint16_t)ble_values.voltage[i];
      val_buffer[(2 * i) + 1] = (uint16_t)ble_values.current[i];
      dev_buffer[(2 * i)] = (uint16_t)ble_values.volt_dev[i];
      dev_buffer[(2 * i) + 1] = (uint16_t)ble_values.curr_dev[i];
    }

    history_fill_buffer(val_buffer, seconds_counter);

    ble_notify_cell_values(val_buffer, VALUES);
    ble_notify_cell_values(dev_buffer, DEVIATIONS);
    memset(&ble_values, 0, sizeof(ble_values));
  }
}