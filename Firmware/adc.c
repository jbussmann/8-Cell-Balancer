#include "adc.h"

#include "board.h"
#include "data.h"
#include "mux.h"
#include "nrfx_saadc.h"
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

static nrf_saadc_value_t samples_buffer[ADC_NUMBER_OF_SAMPLES];

static void saadc_handler(nrfx_saadc_evt_t const *p_event) {
  switch (p_event->type) {
    case NRFX_SAADC_EVT_DONE:  // result of EVT_END, current buffer is filled
      // NRF_LOG_DEBUG("SAADC-DONE event");
      // NRF_SAADC_STATE_ADV_MODE_SAMPLE_STARTED
      data_process_buffer(p_event->data.done.p_buffer);

      // reinitialize adc to prevent sample swaps by clearing excess samples
      nrfx_saadc_uninit();
      adc_init();

      // restart adc when data processing is finished
      mux_pwm_adc_start();
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
