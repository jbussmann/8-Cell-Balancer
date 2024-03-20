#include "adc.h"

#include "config/board.h"
#include "config/sdk_config.h"
#include "mux.h"
#include "nrf_saadc.h"
#include "nrfx_ppi.h"
#include "nrfx_saadc.h"
#include "nrfx_timer.h"

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

static nrfx_timer_t timer1_inst_adc = NRFX_TIMER_INSTANCE(1);

#define NUMBER_OF_CELLS   8
#define NUMBER_OF_SAMPLES 8  // danger of type overflow

typedef struct {
  nrf_saadc_value_t raw_values[NUMBER_OF_SAMPLES];
  // in millivolt/milliampere
  int16_t avg_value_millis;
  int16_t deviation_millis;
} cell_values_t;

typedef struct {
  cell_values_t voltage;
  cell_values_t current;
} cell_t;

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

static void adc_process_buffer(nrfx_saadc_done_evt_t done_event) {
  cell_t cells[NUMBER_OF_CELLS];

  adc_deinterlace_buffer(cells, done_event.p_buffer);
  adc_aggregate_voltage(cells);
  adc_aggregate_current(cells);

  NRF_LOG_INFO("v_low:    %4i, %4i, %4i, %4i",
               cells[0].voltage.avg_value_millis,
               cells[1].voltage.avg_value_millis,
               cells[2].voltage.avg_value_millis,
               cells[3].voltage.avg_value_millis);
  NRF_LOG_INFO("v_high:   %4i, %4i, %4i, %4i",
               cells[4].voltage.avg_value_millis,
               cells[5].voltage.avg_value_millis,
               cells[6].voltage.avg_value_millis,
               cells[7].voltage.avg_value_millis);
  NRF_LOG_INFO("dev_low:  %4i, %4i, %4i, %4i",
               cells[0].voltage.deviation_millis,
               cells[1].voltage.deviation_millis,
               cells[2].voltage.deviation_millis,
               cells[3].voltage.deviation_millis);
  NRF_LOG_INFO("dev_high: %4i, %4i, %4i, %4i",
               cells[4].voltage.deviation_millis,
               cells[5].voltage.deviation_millis,
               cells[6].voltage.deviation_millis,
               cells[7].voltage.deviation_millis);

  // NRF_LOG_INFO("v_low: %i, %i, %i, %i",
  //              cells[0].voltage.raw_values[7],
  //              cells[1].voltage.raw_values[7],
  //              cells[2].voltage.raw_values[7],
  //              cells[3].voltage.raw_values[7]);
  // NRF_LOG_INFO("v_high: %i, %i, %i, %i",
  //              cells[4].voltage.raw_values[7],
  //              cells[5].voltage.raw_values[7],
  //              cells[6].voltage.raw_values[7],
  //              cells[7].voltage.raw_values[7]);
  // NRF_LOG_INFO("c_low: %i, %i, %i, %i",
  //              cells[0].current.raw_values[7],
  //              cells[1].current.raw_values[7],
  //              cells[2].current.raw_values[7],
  //              cells[3].current.raw_values[7]);
  // NRF_LOG_INFO("c_high: %i, %i, %i, %i",
  //              cells[4].current.raw_values[7],
  //              cells[5].current.raw_values[7],
  //              cells[6].current.raw_values[7],
  //              cells[7].current.raw_values[7]);
}

static void saadc_handler(nrfx_saadc_evt_t const *p_event) {
  switch (p_event->type) {
    case NRFX_SAADC_EVT_DONE:  // result of EVT_END, current buffer is filled
      NRF_LOG_DEBUG("SAADC-DONE event");
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
      // NRF_LOG_DEBUG("SAADC-FINISHED event");
      nrfx_timer_clear(&timer1_inst_adc);  // timer is stopped by PPI

      nrfx_err_t status;
      status = nrfx_saadc_buffer_set(samples_buffer, ADC_NUMBER_OF_SAMPLES);
      ERROR_CHECK("SAADC buffer set", status);
      status = nrfx_saadc_mode_trigger();
      ERROR_CHECK("SAADC trigger", status);

      adc_process_buffer(p_event->data.done);
      break;
    default:
      NRF_LOG_WARNING("SAADC unmapped event");
      break;
  }
}

static void adc_init_saadc(void) {
  nrfx_err_t status;

  status = nrfx_saadc_init(NRFX_SAADC_CONFIG_IRQ_PRIORITY);
  ERROR_CHECK("SAADC init", status);

  status = nrfx_saadc_channels_config(channels_config, ADC_CHANNEL_COUNT);
  ERROR_CHECK("SAADC config", status);

  status = nrfx_saadc_offset_calibrate(NULL);  // NULL -> blocking

  // prevent sample swapping (is this true?)
  nrfx_saadc_adv_config_t adv_config = {
      .oversampling = NRF_SAADC_OVERSAMPLE_DISABLED,
      .burst = NRF_SAADC_BURST_DISABLED,
      .internal_timer_cc = 0,
      .start_on_end = false};

  status = nrfx_saadc_advanced_mode_set(
      0b00000011, ADC_RESOLUTON, &adv_config, saadc_handler);
  ERROR_CHECK("SAADC mode set", status);

  status = nrfx_saadc_buffer_set(samples_buffer, ADC_NUMBER_OF_SAMPLES);
  ERROR_CHECK("SAADC buffer set", status);

  status = nrfx_saadc_mode_trigger();
  ERROR_CHECK("SAADC trigger", status);
}

static void adc_init_timer(void) {
  const nrfx_timer_config_t timer_config = {
      .frequency = NRF_TIMER_FREQ_2MHz,
      .mode = NRF_TIMER_MODE_TIMER,
      .bit_width = NRF_TIMER_BIT_WIDTH_32,
      .interrupt_priority = NRFX_TIMER_DEFAULT_CONFIG_IRQ_PRIORITY,
      .p_context = &timer1_inst_adc};
  nrfx_err_t status;

  status = nrfx_timer_init(&timer1_inst_adc, &timer_config, NULL);
  ERROR_CHECK("TIMER init", status);

  nrfx_timer_compare(
      &timer1_inst_adc, NRF_TIMER_CC_CHANNEL0, ADC_SAMPLE_START_TICKS, false);
  nrfx_timer_extended_compare(&timer1_inst_adc,
                              NRF_TIMER_CC_CHANNEL1,
                              ADC_CLEAR_TIMER_TICKS,
                              NRF_TIMER_SHORT_COMPARE1_CLEAR_MASK,
                              false);

  // make sure PPI is not connected yet when this is run
  nrfx_timer_enable(&timer1_inst_adc);  // TIMER starts
  nrfx_timer_pause(&timer1_inst_adc);
  nrfx_timer_clear(&timer1_inst_adc);  // ready for PPI trigger
}

static void adc_init_ppi(void) {
  nrf_ppi_channel_t ppi_channel0;
  nrf_ppi_channel_t ppi_channel1;
  nrfx_err_t status;

  status = nrfx_ppi_channel_alloc(&ppi_channel0);
  ERROR_CHECK("PPI0 alloc", status);
  status = nrfx_ppi_channel_assign(
      ppi_channel0,
      nrfx_timer_compare_event_address_get(&timer1_inst_adc, 0),
      nrf_saadc_task_address_get(NRF_SAADC_TASK_SAMPLE));
  ERROR_CHECK("PPI0 assign", status);
  status = nrfx_ppi_channel_enable(ppi_channel0);
  ERROR_CHECK("PPI0 enable", status);

  status = nrfx_ppi_channel_alloc(&ppi_channel1);
  ERROR_CHECK("PPI1 alloc", status);
  status = nrfx_ppi_channel_assign(
      ppi_channel1,
      nrf_saadc_event_address_get(NRF_SAADC_EVENT_END),
      nrfx_timer_task_address_get(&timer1_inst_adc, NRF_TIMER_TASK_STOP));
  ERROR_CHECK("PPI1 assign", status);
  status = nrfx_ppi_channel_enable(ppi_channel1);
  ERROR_CHECK("PPI1 enable", status);
}

const uint32_t adc_get_timer_task_start(void) {
  return nrfx_timer_task_address_get(&timer1_inst_adc, NRF_TIMER_TASK_START);
}

void adc_init(void) {
  adc_init_saadc();
  adc_init_timer();
  adc_init_ppi();
}
