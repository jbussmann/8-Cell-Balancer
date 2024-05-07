#include "gpio.h"

#include "nrfx_gpiote.h"
#include "pwm.h"

#define NRF_LOG_MODULE_NAME gpio
#include "log.h"
NRF_LOG_MODULE_REGISTER();

static void gpiote_event_handler(nrfx_gpiote_pin_t pin,
                                 nrf_gpiote_polarity_t action) {
  switch (pin) {
    case BUTTON_PIN:
      if (nrf_gpio_pin_read(BUTTON_PIN)) {
        // NRF_LOG_DEBUG("Button released");
      } else {
        pwm_toggle_balancer_state();
        // NRF_LOG_DEBUG("Button pressed");
      }
      break;
    default:
      NRF_LOG_WARNING("undefined event on pin: %i", pin);
      break;
  }
}

static void gpio_init_led_pins(void) {
  const uint8_t leds[] = {LED_R_PIN, LED_G_PIN, LED_B_PIN, TP1_PIN, TP2_PIN};

  for (int i = 0; i < sizeof(leds); i++) {
    nrf_gpio_cfg_output(leds[i]);
    nrf_gpio_pin_set(leds[i]);
  }
}

static void gpio_init_button_pin(void) {
  nrfx_err_t status;
  nrfx_gpiote_in_config_t input_config =
      NRFX_GPIOTE_CONFIG_IN_SENSE_TOGGLE(true);

  nrfx_gpiote_init();
  status = nrfx_gpiote_in_init(BUTTON_PIN, &input_config, gpiote_event_handler);
  ERROR_CHECK("init", status);
  nrfx_gpiote_in_event_enable(BUTTON_PIN, true);
}

void gpio_init(void) {
  gpio_init_led_pins();
  gpio_init_button_pin();
}
