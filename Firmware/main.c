#include "adc.h"
#include "bluetooth.h"
#include "config/board.h"
#include "log.h"
#include "mux.h"
#include "nrf_log_ctrl.h"
#include "nrf_pwr_mgmt.h"
#include "pwm.h"

#define ADVERTISING_LED LED_B_PIN
#define CONNECTED_LED   LED_G_PIN
#define LEDBUTTON_LED   LED_R_PIN

static void gpio_init(void) {
  static const uint8_t leds[] = {
      LED_R_PIN, LED_G_PIN, LED_B_PIN, TP1_PIN, TP2_PIN};

  for (int i = 0; i < sizeof(leds); i++) {
    nrf_gpio_cfg_output(leds[i]);
    nrf_gpio_pin_set(leds[i]);
  }
}

static void idle_state_handle(void) {
  if (NRF_LOG_PROCESS() == false) {
    nrf_pwr_mgmt_run();
  }
}

int main(void) {
  // Initialize.
  log_init();
  gpio_init();

  ble_init();  // creates problems if placed after pwm_start()

  pwm_init();
  mux_init();
  adc_init();

  pwm_start();
  mux_pwm_start();

  // Enter main loop.
  for (;;) {
    idle_state_handle();
  }
}
