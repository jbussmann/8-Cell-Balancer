#include "log.h"

#include "nrf_error.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

void log_init(void) {
  ret_code_t err_code = NRF_LOG_INIT(NULL);
  ERROR_CHECK("LOG init", err_code);
  NRF_LOG_DEFAULT_BACKENDS_INIT();

  NRF_LOG_RAW_INFO(
      "\r\n\e[30;107m\r\n--------------\nLogger "
      "started\n--------------\e[0m\r\n");
}