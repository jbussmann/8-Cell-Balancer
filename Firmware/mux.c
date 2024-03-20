#include "mux.h"

#include "config/board.h"
#include "nrfx_pwm.h"

#define NRF_LOG_MODULE_NAME mux
#include "log.h"
NRF_LOG_MODULE_REGISTER();

#define PWM_TOP_VALUE 25
#define PWM_REPEATS   0
#define PWM_END_DELAY 0
#define PWM_PLAYBACKS 8

#define PWM_HIGH      (0x8000 + PWM_TOP_VALUE)
#define PWM_LOW       (0x8000 + 0)

static uint32_t pwm_start_address;

static nrf_pwm_values_individual_t pwm_value[] = {
    {PWM_HIGH, PWM_HIGH, PWM_LOW, 0},
    {PWM_LOW, PWM_LOW, PWM_LOW, 0},
    {PWM_HIGH, PWM_LOW, PWM_LOW, 0},
    {PWM_LOW, PWM_HIGH, PWM_LOW, 0},
    {PWM_HIGH, PWM_LOW, PWM_HIGH, 0},
    {PWM_LOW, PWM_HIGH, PWM_HIGH, 0},
    {PWM_HIGH, PWM_HIGH, PWM_HIGH, 0},
    {PWM_LOW, PWM_LOW, PWM_HIGH, 0}};

static const nrfx_pwm_t pwm2_instance_mux = NRFX_PWM_INSTANCE(2);

static const nrf_pwm_sequence_t pwm_sequence = {
    .values.p_individual = pwm_value,
    .length = NRF_PWM_VALUES_LENGTH(pwm_value),
    .repeats = PWM_REPEATS,
    .end_delay = PWM_END_DELAY};

static void mux_init_pwm(void) {
  const nrfx_pwm_config_t pwm_config = {
      .output_pins = {MUX_S0, MUX_S1, MUX_S2, NRFX_PWM_PIN_NOT_USED},
      .irq_priority = NRFX_PWM_DEFAULT_CONFIG_IRQ_PRIORITY,
      .base_clock = NRF_PWM_CLK_2MHz,
      .count_mode = NRF_PWM_MODE_UP,
      .top_value = PWM_TOP_VALUE,
      .load_mode = NRF_PWM_LOAD_INDIVIDUAL,
      .step_mode = NRF_PWM_STEP_AUTO};

  nrfx_err_t status = nrfx_pwm_init(&pwm2_instance_mux, &pwm_config, NULL);
  ERROR_CHECK("PWM init", status);

  uint32_t flags = NRFX_PWM_FLAG_STOP | NRFX_PWM_FLAG_START_VIA_TASK;
  pwm_start_address = nrfx_pwm_simple_playback(
      &pwm2_instance_mux, &pwm_sequence, PWM_PLAYBACKS, flags);
}

uint32_t mux_get_pwm_task_start() { return pwm_start_address; }

void mux_init(void) {
  nrf_gpio_cfg_output(MUX_EN);
  nrf_gpio_pin_clear(MUX_EN);

  mux_init_pwm();
}
