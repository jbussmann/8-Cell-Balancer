#ifndef BOARD_H
#define BOARD_H

#include "nrf_gpio.h"

#define I_SET_PIN   NRF_GPIO_PIN_MAP(0, 13)

#define EN_3V3_PIN  NRF_GPIO_PIN_MAP(0, 15)

#define LED_R_PIN   NRF_GPIO_PIN_MAP(0, 17)
#define LED_G_PIN   NRF_GPIO_PIN_MAP(0, 19)
#define LED_B_PIN   NRF_GPIO_PIN_MAP(0, 20)

// also set NRF_LOG_BACKEND_UART_TX_PIN!
#define UART_TX_PIN NRF_GPIO_PIN_MAP(1, 12)
#define UART_RX_PIN NRF_GPIO_PIN_MAP(1, 13)

#endif  // BOARD_H
