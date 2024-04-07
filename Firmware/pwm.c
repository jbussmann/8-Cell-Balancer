#include "pwm.h"

#include "adc.h"
#include "config/board.h"
#include "config/sdk_config.h"
#include "mux.h"
#include "nrfx_ppi.h"
#include "nrfx_pwm.h"

#define NRF_LOG_MODULE_NAME pwm
#include "log.h"
NRF_LOG_MODULE_REGISTER();

#define PWM_TOP_VALUE 100
#define PWM_REPEATS   0
#define PWM_END_DELAY 0
#define PWM_PLAYBACKS 1

static nrf_pwm_values_individual_t pwm_value_14[] = {
    {0x8000 + 5, 0x8000 + 5, 0x8000 + 5, 0x8000 + 5}};
static nrf_pwm_values_individual_t pwm_value_58[] = {
    {0x8000 + 5, 0x8000 + 5, 0x8000 + 5, 0x8000 + 5}};

static const nrfx_pwm_t pwm0_instance_bal14 = NRFX_PWM_INSTANCE(0);
static const nrfx_pwm_t pwm1_instance_bal58 = NRFX_PWM_INSTANCE(1);

static const nrf_pwm_sequence_t pwm_sequence_14 = {
    .values.p_individual = pwm_value_14,
    .length = NRF_PWM_VALUES_LENGTH(pwm_value_14),
    .repeats = PWM_REPEATS,
    .end_delay = PWM_END_DELAY};
static const nrf_pwm_sequence_t pwm_sequence_58 = {
    .values.p_individual = pwm_value_58,
    .length = NRF_PWM_VALUES_LENGTH(pwm_value_58),
    .repeats = PWM_REPEATS,
    .end_delay = PWM_END_DELAY};

void pwm_update_values(uint16_t values[]) {
  pwm_value_14->channel_0 = 0x8000 + values[0];
  pwm_value_14->channel_1 = 0x8000 + values[1];
  pwm_value_14->channel_2 = 0x8000 + values[2];
  pwm_value_14->channel_3 = 0x8000 + values[3];
  pwm_value_58->channel_0 = 0x8000 + values[4];
  pwm_value_58->channel_1 = 0x8000 + values[5];
  pwm_value_58->channel_2 = 0x8000 + values[6];
  pwm_value_58->channel_3 = 0x8000 + values[7];
}

void pwm_start(void) {
  nrfx_pwm_simple_playback(&pwm0_instance_bal14,
                           &pwm_sequence_14,
                           PWM_PLAYBACKS,
                           NRFX_PWM_FLAG_LOOP);
  nrfx_pwm_simple_playback(&pwm1_instance_bal58,
                           &pwm_sequence_58,
                           PWM_PLAYBACKS,
                           NRFX_PWM_FLAG_LOOP);
}

void pwm_init(void) {
  const nrfx_pwm_config_t pwm_config_14 = {
      .output_pins = {BAL1_PIN, BAL2_PIN, BAL3_PIN, BAL4_PIN},
      .irq_priority = NRFX_PWM_DEFAULT_CONFIG_IRQ_PRIORITY,
      .base_clock = NRF_PWM_CLK_1MHz,
      .count_mode = NRF_PWM_MODE_UP,
      .top_value = PWM_TOP_VALUE,
      .load_mode = NRF_PWM_LOAD_INDIVIDUAL,
      .step_mode = NRF_PWM_STEP_AUTO};

  nrfx_err_t status = nrfx_pwm_init(&pwm0_instance_bal14, &pwm_config_14, NULL);
  ERROR_CHECK("PWM0 init", status);

  const nrfx_pwm_config_t pwm_config_58 = {
      .output_pins = {BAL5_PIN, BAL6_PIN, BAL7_PIN, BAL8_PIN},
      .irq_priority = NRFX_PWM_DEFAULT_CONFIG_IRQ_PRIORITY,
      .base_clock = NRF_PWM_CLK_1MHz,
      .count_mode = NRF_PWM_MODE_UP,
      .top_value = PWM_TOP_VALUE,
      .load_mode = NRF_PWM_LOAD_INDIVIDUAL,
      .step_mode = NRF_PWM_STEP_AUTO};

  status = nrfx_pwm_init(&pwm1_instance_bal58, &pwm_config_58, NULL);
  ERROR_CHECK("PWM1 init", status);
}
