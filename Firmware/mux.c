#include "mux.h"

#include "config/board.h"

static uint8_t counter = 0;

// reset counter in case NRFX_SAADC_EVT_FINISHED arrives after
// NRF_TIMER_EVENT_COMPARE0
void mux_reset(void) {
  counter = 7;
  mux_set();
}

void mux_init(void) {
  static const uint8_t mux_pins[] = {MUX_S0, MUX_S1, MUX_S2, MUX_EN};

  for (int i = 0; i < sizeof(mux_pins); i++) {
    nrf_gpio_cfg_output(mux_pins[i]);
    nrf_gpio_pin_clear(mux_pins[i]);
  }
}

void mux_set(void) {
  static const uint8_t output[] = {
      0b011,  // C5 V1
      0b000,  // C6 V2
      0b001,  // C7 V3
      0b010,  // C8 V4
      0b101,  // V5 C1
      0b110,  // V7 C3
      0b111,  // V6 C2
      0b100,  // V8 C4
  };

  nrf_gpio_pin_write(MUX_S0, output[counter] & 0b001);
  nrf_gpio_pin_write(MUX_S1, output[counter] & 0b010);
  nrf_gpio_pin_write(MUX_S2, output[counter] & 0b100);

  if (7 <= counter) {
    counter = 0;
  } else {
    counter++;
  }
}
