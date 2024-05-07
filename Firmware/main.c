#include "adc.h"
#include "bluetooth.h"
#include "gpio.h"
#include "log.h"
#include "mux.h"
#include "nrf_log_ctrl.h"
#include "nrf_pwr_mgmt.h"
#include "pwm.h"

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
