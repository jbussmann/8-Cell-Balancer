#ifndef BOARD_H
#define BOARD_H

#include "nrf_gpio.h"
#include "nrf_saadc.h"

// #define LOWER_MUX_ADC NRF_GPIO_PIN_MAP(0, 4)
// #define UPPER_MUX_ADC NRF_GPIO_PIN_MAP(0, 5)
#define LOWER_MUX_ADC NRF_SAADC_INPUT_AIN2
#define UPPER_MUX_ADC NRF_SAADC_INPUT_AIN3

#define MUX_S0        NRF_GPIO_PIN_MAP(0, 29)
#define MUX_S1        NRF_GPIO_PIN_MAP(0, 30)
#define MUX_S2        NRF_GPIO_PIN_MAP(0, 23)
#define MUX_EN        NRF_GPIO_PIN_MAP(0, 21)

#define BAL1_PIN      NRF_GPIO_PIN_MAP(0, 3)
#define BAL2_PIN      NRF_GPIO_PIN_MAP(0, 28)
#define BAL3_PIN      NRF_GPIO_PIN_MAP(0, 2)
#define BAL4_PIN      NRF_GPIO_PIN_MAP(1, 12)
#define BAL5_PIN      NRF_GPIO_PIN_MAP(0, 15)
#define BAL6_PIN      NRF_GPIO_PIN_MAP(0, 16)
#define BAL7_PIN      NRF_GPIO_PIN_MAP(0, 24)
#define BAL8_PIN      NRF_GPIO_PIN_MAP(0, 25)

#define LED_R_PIN     NRF_GPIO_PIN_MAP(0, 20)
#define LED_G_PIN     NRF_GPIO_PIN_MAP(0, 17)
#define LED_B_PIN     NRF_GPIO_PIN_MAP(0, 19)

#define ADVERTISE_LED LED_B_PIN
#define CONNECTED_LED LED_G_PIN
#define BALANCING_LED LED_R_PIN

#define BUTTON_PIN    NRF_GPIO_PIN_MAP(0, 13)

// also set NRF_LOG_BACKEND_UART_TX_PIN!
#define UART_TX_PIN   NRF_GPIO_PIN_MAP(1, 13)

#define TP1_PIN       NRF_GPIO_PIN_MAP(0, 11)
#define TP2_PIN       NRF_GPIO_PIN_MAP(0, 14)

#endif  // BOARD_H
