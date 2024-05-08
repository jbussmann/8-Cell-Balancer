#include "history.h"

#include "ble_services.h"
#include "data.h"

#define HISTORY_BUFFER_ELEMENTS  120
#define HISTORY_1H_INTERVALL     30
#define HISTORY_12H_INTERVALL    360
#define HISTORY_12H_1H_INTERVALL (HISTORY_12H_INTERVALL / HISTORY_1H_INTERVALL)

#define NRF_LOG_MODULE_NAME      history
#include "log.h"
NRF_LOG_MODULE_REGISTER();

static uint16_t history_2min_buffer[HISTORY_BUFFER_ELEMENTS][16] = {
    [0 ... HISTORY_BUFFER_ELEMENTS - 1] = {[0 ... 15] = 0xffff}};
static uint16_t history_1h_buffer[HISTORY_BUFFER_ELEMENTS][16] = {
    [0 ... HISTORY_BUFFER_ELEMENTS - 1] = {[0 ... 15] = 0xffff}};
static uint16_t history_12h_buffer[HISTORY_BUFFER_ELEMENTS][16] = {
    [0 ... HISTORY_BUFFER_ELEMENTS - 1] = {[0 ... 15] = 0xffff}};

// position of most recent data point
static uint8_t history_2min_head = HISTORY_BUFFER_ELEMENTS - 1;
static uint8_t history_1h_head = HISTORY_BUFFER_ELEMENTS - 1;
static uint8_t history_12h_head = HISTORY_BUFFER_ELEMENTS - 1;

void history_notify_1h(void) {
  uint16_t values[96] = {[0 ... 95] = 42};
  for (size_t i = 0; i < 20; i++) {
    ble_notify_history_values(values, HISTORY_1H);
  }
}

// void history_notify_12h(void) {
//   uint16_t values[96] = {[0 ... 95] = 42};
//   for (size_t i = 0; i < 20; i++) {
//     ble_notify_history_values(values, HISTORY_12H);
//   }
// }

static void history_aggregate_1h(void) {
  history_1h_head++;
  history_1h_head %= HISTORY_BUFFER_ELEMENTS;

  for (size_t i = 0; i < NUMBER_OF_CELLS; i++) {
    uint32_t sum_v = 0;
    uint32_t sum_i = 0;
    for (size_t k = 0; k < HISTORY_1H_INTERVALL; k++) {
      sum_v += history_2min_buffer[history_2min_head - k][i];
      sum_i += history_2min_buffer[history_2min_head - k][i + 1];
    }
    history_1h_buffer[history_1h_head][i] = sum_v / HISTORY_1H_INTERVALL;
    history_1h_buffer[history_1h_head][i + 1] = sum_i / HISTORY_1H_INTERVALL;
  }
}

static void history_aggregate_12h(void) {
  history_12h_head++;
  history_12h_head %= HISTORY_BUFFER_ELEMENTS;
  for (size_t i = 0; i < NUMBER_OF_CELLS; i++) {
    uint32_t sum_v = 0;
    uint32_t sum_i = 0;
    for (size_t k = 0; k < HISTORY_12H_1H_INTERVALL; k++) {
      sum_v += history_1h_buffer[history_1h_head - k][i];
      sum_i += history_1h_buffer[history_1h_head - k][i + 1];
    }
    history_12h_buffer[history_12h_head][i] = sum_v / HISTORY_12H_1H_INTERVALL;
    history_12h_buffer[history_12h_head][i + 1] =
        sum_i / HISTORY_12H_1H_INTERVALL;
  }
}

void history_fill_buffer(uint16_t values_buffer[2 * NUMBER_OF_CELLS],
                         uint16_t seconds) {
  history_2min_head++;
  history_2min_head %= HISTORY_BUFFER_ELEMENTS;
  uint16_t *p_head = &history_2min_buffer[history_2min_head][0];
  for (size_t i = 0; i < 2 * NUMBER_OF_CELLS; i++) {
    *(p_head + i) = values_buffer[i];
  }
  NRF_LOG_INFO("%i: 2min buffer added @ pos%i", seconds, history_2min_head);

  if (seconds % HISTORY_1H_INTERVALL == 0) {
    history_aggregate_1h();
    NRF_LOG_INFO("%i: 1h buffer added @ pos%i", seconds, history_1h_head);
  }

  if (seconds % HISTORY_12H_INTERVALL == 0) {
    history_aggregate_12h();
    NRF_LOG_INFO("%i: 12h buffer added @ pos%i", seconds, history_12h_head);
  }
}
