#include "adc.h"
#include "bluetooth.h"
#include "config/board.h"
#include "log.h"
#include "nrf_log_ctrl.h"
#include "nrf_pwr_mgmt.h"

#define ADVERTISING_LED LED_B_PIN
#define CONNECTED_LED   LED_G_PIN
#define LEDBUTTON_LED   LED_R_PIN

static void leds_init(void) {
  static const uint8_t leds[] = {LED_R_PIN, LED_G_PIN, LED_B_PIN};

  for (int i = 0; i < sizeof(leds); i++) {
    nrf_gpio_cfg_output(leds[i]);
    nrf_gpio_pin_set(leds[i]);
  }
  nrf_gpio_cfg_output(EN_3V3_PIN);
  nrf_gpio_pin_set(EN_3V3_PIN);
}

static void idle_state_handle(void) {
  if (NRF_LOG_PROCESS() == false) {
    nrf_pwr_mgmt_run();
  }
}

int main(void) {
  // Initialize.
  log_init();
  leds_init();

  ble_init();

  adc_init();

  // Enter main loop.
  for (;;) {
    idle_state_handle();
  }
}
