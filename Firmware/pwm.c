#include "pwm.h"

#include "adc.h"
#include "board.h"
#include "mux.h"
#include "nrfx_ppi.h"
#include "nrfx_pwm.h"
#include "sdk_config.h"

#define NRF_LOG_MODULE_NAME pwm
#include "log.h"
NRF_LOG_MODULE_REGISTER();

#define CHARGE_TERM_VOLT_MAX 3600
#define CHARGE_TERM_HYST     10
#define CHARGE_TERM_RANGE    20

#define PWM_TOP_VALUE        100
#define PWM_REPEATS          0
#define PWM_END_DELAY        0
#define PWM_PLAYBACKS        1

#define PWM_LIMIT            75

static uint16_t pwm_current_values[8] = {0};
static uint16_t pwm_term_volt_individual[8] = {CHARGE_TERM_VOLT_MAX};

static bool is_balancing_active = false;

static nrf_pwm_values_individual_t pwm_value_14[] = {
    {0x8000, 0x8000, 0x8000, 0x8000}};
static nrf_pwm_values_individual_t pwm_value_58[] = {
    {0x8000, 0x8000, 0x8000, 0x8000}};

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

static void pwm_apply_values() {
  pwm_value_14->channel_0 = 0x8000 + pwm_current_values[0];
  pwm_value_14->channel_1 = 0x8000 + pwm_current_values[1];
  pwm_value_14->channel_2 = 0x8000 + pwm_current_values[2];
  pwm_value_14->channel_3 = 0x8000 + pwm_current_values[3];
  pwm_value_58->channel_0 = 0x8000 + pwm_current_values[4];
  pwm_value_58->channel_1 = 0x8000 + pwm_current_values[5];
  pwm_value_58->channel_2 = 0x8000 + pwm_current_values[6];
  pwm_value_58->channel_3 = 0x8000 + pwm_current_values[7];
}

void pwm_toggle_balancer_state() {
  if (is_balancing_active) {
    memset(pwm_current_values, 0, sizeof(pwm_current_values));
    pwm_apply_values();
    nrf_gpio_pin_set(BALANCING_LED);  // off
    NRF_LOG_INFO("balancer disabled");
  } else {
    nrf_gpio_pin_clear(BALANCING_LED);  // on
    NRF_LOG_INFO("balancer enabled");
  }
  is_balancing_active = !is_balancing_active;
}

void pwm_update_values(uint16_t values[8]) {
  for (size_t i = 0; i < ARRAY_SIZE(pwm_current_values); i++) {
    pwm_current_values[i] = values[i];
  }
  pwm_apply_values();
}

void pwm_calculate_term_volt(uint16_t voltages[8]) {
  uint16_t lowest_voltage = 5000;
  for (size_t i = 0; i < 8; i++) {
    lowest_voltage = MIN(voltages[i], lowest_voltage);
  }

  uint16_t upper_bound =
      MIN(lowest_voltage + CHARGE_TERM_RANGE, CHARGE_TERM_VOLT_MAX);
  for (size_t i = 0; i < 8; i++) {
    pwm_term_volt_individual[i] = MAX(upper_bound, pwm_term_volt_individual[i]);
  }
}

void pwm_calculate_next_values(uint16_t voltages[8]) {
  if (is_balancing_active) {
    pwm_calculate_term_volt(voltages);
    for (size_t i = 0; i < ARRAY_SIZE(pwm_current_values); i++) {
      if (((pwm_term_volt_individual[i] + CHARGE_TERM_HYST) < voltages[i]) &&
          (pwm_current_values[i] < PWM_LIMIT)) {
        pwm_current_values[i]++;
      } else if ((voltages[i] <
                  (pwm_term_volt_individual[i] - CHARGE_TERM_HYST)) &&
                 (0 < pwm_current_values[i])) {
        pwm_current_values[i]--;
      }
    }
    pwm_apply_values();
  } else {
    for (size_t i = 0; i < 8; i++) {
      pwm_term_volt_individual[i] = MIN(voltages[i], CHARGE_TERM_VOLT_MAX);
    }
  }
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
