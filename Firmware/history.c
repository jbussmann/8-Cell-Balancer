#include "history.h"

#include "ble_services.h"
#include "data.h"

#define HISTORY_BUFFER_ELEMENTS 120
#define HISTORY_1H_INTERVAL     30
#define HISTORY_12H_INTERVAL    360
#define HISTORY_12H_1H_INTERVAL (HISTORY_12H_INTERVAL / HISTORY_1H_INTERVAL)

#define NRF_LOG_MODULE_NAME     history
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

static void history_notify(uint16_t p_buffer[][16],
                           uint8_t* p_head,
                           uint8_t type,
                           bool full) {
  uint16_t values[BLE_HISTORY_CHAR_LENGTH / sizeof(uint16_t)] = {[0 ... 95] =
                                                                     0xffff};

  if (full) {
    for (size_t i = 0; i < HISTORY_BUFFER_ELEMENTS; i++) {
      uint8_t pos = *p_head + i + 1;
      pos %= HISTORY_BUFFER_ELEMENTS;

      memcpy(&values[16 * (i % 6)], p_buffer[pos], sizeof(uint16_t) * 16);

      if (i % 6 == 5) {
        ble_notify_history_values(values, type);
      }
    }
  } else {
    memcpy(values, p_buffer[*p_head], sizeof(uint16_t) * 16);
    ble_notify_history_values(values, type);
  }
}

void history_notify_1h_full() {
  history_notify(history_1h_buffer, &history_1h_head, HISTORY_1H, true);
}

void history_notify_12h_full() {
  history_notify(history_12h_buffer, &history_12h_head, HISTORY_12H, true);
}

static void history_aggregate(uint16_t p_srcbuff[][16],
                              uint8_t* p_srchead,
                              uint16_t p_dstbuff[][16],
                              uint8_t* p_dsthead,
                              uint16_t interval) {
  *p_dsthead += 1;
  *p_dsthead %= HISTORY_BUFFER_ELEMENTS;

  for (size_t i = 0; i < NUMBER_OF_CELLS; i++) {
    uint32_t sum_v = 0;
    uint32_t sum_i = 0;
    for (size_t k = 0; k < interval; k++) {
      sum_v += p_srcbuff[*p_srchead - k][2 * i];  // underflow?
      sum_i += p_srcbuff[*p_srchead - k][2 * i + 1];
    }
    p_dstbuff[*p_dsthead][2 * i] = sum_v / interval;
    p_dstbuff[*p_dsthead][2 * i + 1] = sum_i / interval;
  }
}

void history_fill_buffer(uint16_t values_buffer[2 * NUMBER_OF_CELLS],
                         uint16_t seconds) {
  history_2min_head++;
  history_2min_head %= HISTORY_BUFFER_ELEMENTS;
  uint16_t* p_head = &history_2min_buffer[history_2min_head][0];
  for (size_t i = 0; i < 2 * NUMBER_OF_CELLS; i++) {
    *(p_head + i) = values_buffer[i];
  }
  NRF_LOG_INFO("%i: 2min buffer added @ pos%i", seconds, history_2min_head);

  if (seconds % HISTORY_1H_INTERVAL == 0) {
    history_aggregate(history_2min_buffer,
                      &history_2min_head,
                      history_1h_buffer,
                      &history_1h_head,
                      (uint16_t)HISTORY_1H_INTERVAL);
    history_notify(history_1h_buffer, &history_1h_head, HISTORY_1H, false);
    NRF_LOG_INFO("%i: 1h buffer added @ pos%i", seconds, history_1h_head);
  }

  if (seconds % HISTORY_12H_INTERVAL == 0) {
    history_aggregate(history_1h_buffer,
                      &history_1h_head,
                      history_12h_buffer,
                      &history_12h_head,
                      (uint16_t)HISTORY_12H_1H_INTERVAL);
    history_notify(history_12h_buffer, &history_12h_head, HISTORY_12H, false);
    NRF_LOG_INFO("%i: 12h buffer added @ pos%i", seconds, history_12h_head);
  }
}
