#include "adc.h"

#include "nrf_saadc.h"
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
            .gain = NRF_SAADC_GAIN1_4,                                     \
            .reference = NRF_SAADC_REFERENCE_VDD4,                         \
            .acq_time = NRF_SAADC_ACQTIME_3US,                             \
            .mode = NRF_SAADC_MODE_SINGLE_ENDED,                           \
            .burst = NRF_SAADC_BURST_DISABLED,                             \
        },                                                                 \
    .pin_p = (nrf_saadc_input_t)_pin_p, .pin_n = NRF_SAADC_INPUT_DISABLED, \
    .channel_index = _index,                                               \
  }

static const nrfx_saadc_channel_t channels_config[] = {
    // SAADC_CHANNEL_CONF(NRF_SAADC_INPUT_AIN2, 0),
    // SAADC_CHANNEL_CONF(NRF_SAADC_INPUT_AIN3, 1),
    SAADC_CHANNEL_CONF(NRF_SAADC_INPUT_AIN7, 0),
    SAADC_CHANNEL_CONF(NRF_SAADC_INPUT_AIN5, 1),
};
#define ADC_CHANNEL_COUNT     NRFX_ARRAY_SIZE(channels_config)

#define ADC_SET_MUX_TICKS     5
#define ADC_CLEAR_TIMER_TICKS 25

#define ADC_NUMBER_OF_SAMPLES 128  // 8x 16 Signals

#define ADC_RESOLUTON         NRF_SAADC_RESOLUTION_14BIT
#define ADC_RESOLUTON_BITS    (8 + (2 * ADC_RESOLUTON))
#define ADC_RANGE             (1 << ADC_RESOLUTON_BITS)

static nrf_saadc_value_t samples_buffer[ADC_NUMBER_OF_SAMPLES];

static nrfx_timer_t timer_inst = NRFX_TIMER_INSTANCE(1);

void adc_set_mux() {}

void adc_process_buffer() {}

static void saadc_handler(nrfx_saadc_evt_t const *p_event) {
  switch (p_event->type) {
    case NRFX_SAADC_EVT_DONE:  // result of EVT_END, current buffer is filled
      // NRF_LOG_DEBUG("SAADC-DONE event");
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
      NRF_LOG_DEBUG("SAADC-FINISHED event");
      nrfx_timer_pause(&timer_inst);
      NRF_LOG_INFO("Buffer has %u samples", p_event->data.done.size)
      adc_process_buffer();
      break;
    default:
      NRF_LOG_WARNING("SAADC unmapped event");
      break;
  }
}

static void timer_handler(nrf_timer_event_t event_type, void *p_context) {
  switch (event_type) {
    case NRF_TIMER_EVENT_COMPARE0:
      // NRF_LOG_DEBUG("TIMER-COMPARE0 event");
      adc_set_mux();
      nrf_saadc_task_trigger(NRF_SAADC_TASK_SAMPLE);
      break;
    default:
      NRF_LOG_WARNING("TIMER unmapped event");
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
      .p_context = &timer_inst};
  nrfx_err_t status;

  status = nrfx_timer_init(&timer_inst, &timer_config, timer_handler);
  ERROR_CHECK("TIMER init", status);

  nrfx_timer_compare(
      &timer_inst, NRF_TIMER_CC_CHANNEL0, ADC_SET_MUX_TICKS, true);
  nrfx_timer_extended_compare(&timer_inst,
                              NRF_TIMER_CC_CHANNEL1,
                              ADC_CLEAR_TIMER_TICKS,
                              NRF_TIMER_SHORT_COMPARE1_CLEAR_MASK,
                              false);

  nrfx_timer_enable(&timer_inst);  // TIMER starts
  nrfx_timer_pause(&timer_inst);
  nrfx_timer_clear(&timer_inst);  // ready for PPI trigger
}

const uint32_t adc_get_task(void) {
  return nrfx_timer_task_address_get(&timer_inst, NRF_TIMER_TASK_START);
}

void adc_init(void) {
  adc_init_saadc();
  adc_init_timer();
}
