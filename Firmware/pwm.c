#include "pwm.h"

#include "adc.h"
#include "config/board.h"
#include "config/sdk_config.h"
#include "nrfx_ppi.h"
#include "nrfx_pwm.h"

#define NRF_LOG_MODULE_NAME pwm
#include "log.h"
NRF_LOG_MODULE_REGISTER();

#define PWM_INSTANCE  0
#define PWM_TOP_VALUE 100
#define PWM_REPEATS   0
#define PWM_END_DELAY 0
#define PWM_PLAYBACKS 10

static nrf_pwm_values_common_t pwm_value[] = {0x8000 + 5,
                                               0x8000 + 15,
                                               0x8000 + 25,
                                               0x8000 + 35,
                                               0x8000 + 45,
                                               0x8000 + 55,
                                               0x8000 + 65,
                                               0x8000 + 75,
                                               0x8000 + 85,
                                               0x8000 + 95};

const nrfx_pwm_t pwm_instance = NRFX_PWM_INSTANCE(PWM_INSTANCE);

const nrf_pwm_sequence_t pwm_sequence = {
    .values.p_common = pwm_value,
    .length = NRF_PWM_VALUES_LENGTH(pwm_value),
    .repeats = PWM_REPEATS,
    .end_delay = PWM_END_DELAY};

// static void pwm_invert_values(nrf_pwm_values_common_t* values,
//                               uint16_t length) {
//   for (size_t i = 0; i < length; i++) {
//     values[i] |= 0x8000;
//   }
// }

void pwm_start(void) {
  uint32_t flags = NRFX_PWM_FLAG_SIGNAL_END_SEQ0 |
                   NRFX_PWM_FLAG_SIGNAL_END_SEQ1;
  nrfx_err_t status = nrfx_pwm_simple_playback(
      &pwm_instance, &pwm_sequence, PWM_PLAYBACKS, flags);
  ERROR_CHECK("PWM simple playback", status);
}

static void pwm_handler(nrfx_pwm_evt_type_t event_type) {
  switch (event_type) {
    case NRFX_PWM_EVT_FINISHED: // result of EVENTS_LOOPSDONE
      NRF_LOG_DEBUG("PWM-FINISHED event");
      break;
    case NRFX_PWM_EVT_END_SEQ0:
      NRF_LOG_DEBUG("PWM-ENDSEQ0 event");
      break;
    case NRFX_PWM_EVT_END_SEQ1:
      NRF_LOG_DEBUG("PWM-ENDSEQ1 event");
      break;
    case NRFX_PWM_EVT_STOPPED:
      NRF_LOG_DEBUG("PWM-STOPPED event");
      break;
    default:
      NRF_LOG_WARNING("PWM unmapped event");
      break;
  }
}

static void pwm_init_pwm(void) {
  const nrfx_pwm_config_t pwm_config = {
      .output_pins = {BAL1_PIN, BAL2_PIN, BAL3_PIN, BAL4_PIN},
      .irq_priority = NRFX_PWM_DEFAULT_CONFIG_IRQ_PRIORITY,
      .base_clock = NRF_PWM_CLK_1MHz,
      .count_mode = NRF_PWM_MODE_UP,
      .top_value = PWM_TOP_VALUE,
      .load_mode = NRF_PWM_LOAD_COMMON,
      .step_mode = NRF_PWM_STEP_AUTO};

  nrfx_err_t status = nrfx_pwm_init(&pwm_instance, &pwm_config, pwm_handler);
  ERROR_CHECK("PWM init", status);
}

static void pwm_init_ppi(void) {
  nrf_ppi_channel_t ppi_channel0;
  nrf_ppi_channel_t ppi_channel1;
  nrfx_err_t status;

  status = nrfx_ppi_channel_alloc(&ppi_channel0);
  ERROR_CHECK("PPI0 alloc", status);
  status = nrfx_ppi_channel_alloc(&ppi_channel1);
  ERROR_CHECK("PPI1 alloc", status);

  const uint32_t event_address0 =
      nrfx_pwm_event_address_get(&pwm_instance, NRF_PWM_EVENT_SEQSTARTED0);
  const uint32_t event_address1 =
      nrfx_pwm_event_address_get(&pwm_instance, NRF_PWM_EVENT_SEQSTARTED1);
  const uint32_t task_address = adc_get_task();

  status = nrfx_ppi_channel_assign(ppi_channel0, event_address0, task_address);
  ERROR_CHECK("PPI0 assign", status);
  status = nrfx_ppi_channel_assign(ppi_channel1, event_address1, task_address);
  ERROR_CHECK("PPI1 assign", status);

  status = nrfx_ppi_channel_enable(ppi_channel0);
  ERROR_CHECK("PPI0 enable", status);
  status = nrfx_ppi_channel_enable(ppi_channel1);
  ERROR_CHECK("PPI1 enable", status);
}

void pwm_init(void) {
  pwm_init_pwm();
  pwm_init_ppi();
}