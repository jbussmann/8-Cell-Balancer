#include "mux.h"

#include "board.h"
#include "nrfx_ppi.h"
#include "nrfx_pwm.h"

#define NRF_LOG_MODULE_NAME mux
#include "log.h"
NRF_LOG_MODULE_REGISTER();

#define PWM_TOP_VALUE 25
#define PWM_REPEATS   0
#define PWM_END_DELAY 0
#define PWM_PLAYBACKS 1  // loop flag is set

#define PWM_HIGH      (0x8000 + PWM_TOP_VALUE)
#define PWM_LOW       (0x8000 + 0)

// pin sequence: S0, S1, S2
static nrf_pwm_values_individual_t pwm_value[] = {
    {PWM_LOW, PWM_LOW, PWM_HIGH, 0},     // 8
    {PWM_HIGH, PWM_HIGH, PWM_LOW, 0},    // 1
    {PWM_LOW, PWM_LOW, PWM_LOW, 0},      // 2
    {PWM_HIGH, PWM_LOW, PWM_LOW, 0},     // 3
    {PWM_LOW, PWM_HIGH, PWM_LOW, 0},     // 4
    {PWM_HIGH, PWM_LOW, PWM_HIGH, 0},    // 5
    {PWM_LOW, PWM_HIGH, PWM_HIGH, 0},    // 6
    {PWM_HIGH, PWM_HIGH, PWM_HIGH, 0}};  // 7

static const nrfx_pwm_t pwm2_instance_mux = NRFX_PWM_INSTANCE(2);

static const nrf_pwm_sequence_t pwm_sequence = {
    .values.p_individual = pwm_value,
    .length = NRF_PWM_VALUES_LENGTH(pwm_value),
    .repeats = PWM_REPEATS,
    .end_delay = PWM_END_DELAY};

void mux_pwm_adc_start(void) {
  // PPI triggers SAADC sampling
  nrfx_pwm_simple_playback(
      &pwm2_instance_mux, &pwm_sequence, PWM_PLAYBACKS, NRFX_PWM_FLAG_LOOP);
}

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
}

static void mux_init_adc_sample_ppi(void) {
  nrf_ppi_channel_t ppi_channel;
  nrfx_err_t status;

  status = nrfx_ppi_channel_alloc(&ppi_channel);
  ERROR_CHECK("PPI adc sample alloc", status);

  status = nrfx_ppi_channel_assign(
      ppi_channel,
      nrfx_pwm_event_address_get(&pwm2_instance_mux,
                                 NRF_PWM_EVENT_PWMPERIODEND),
      nrf_saadc_task_address_get(NRF_SAADC_TASK_SAMPLE));
  ERROR_CHECK("PPI adc sample assign", status);
  status = nrfx_ppi_channel_enable(ppi_channel);
  ERROR_CHECK("PPI adc sample enable", status);
}

static void mux_init_pwm_stop_ppi(void) {
  nrf_ppi_channel_t ppi_channel;
  nrfx_err_t status;

  status = nrfx_ppi_channel_alloc(&ppi_channel);
  ERROR_CHECK("PPI pwm stop alloc", status);

  status = nrfx_ppi_channel_assign(
      ppi_channel,
      nrf_saadc_event_address_get(NRF_SAADC_EVENT_END),
      nrfx_pwm_event_address_get(&pwm2_instance_mux, NRF_PWM_TASK_STOP));
  ERROR_CHECK("PPI pwm stop assign", status);
  status = nrfx_ppi_channel_enable(ppi_channel);
  ERROR_CHECK("PPI pwm stop enable", status);
}

void mux_init(void) {
  nrf_gpio_cfg_output(MUX_EN);
  nrf_gpio_pin_clear(MUX_EN);

  mux_init_pwm();
  mux_init_adc_sample_ppi();
  mux_init_pwm_stop_ppi();
}
