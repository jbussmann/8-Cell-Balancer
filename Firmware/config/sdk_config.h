#ifndef SDK_CONFIG_H
#define SDK_CONFIG_H

// #include "config/board.h"

// Accessibility of peripherals under S140
// Blocked:
// RADIO, TIMER0, RTC0, CCM, AAR, EGU5/SWI5, FICR
// Restricted:
// CLOCK, POWER, TEMP, RNG, ECB, EGU1/SWI1/Radio Notification, ACL, NVMC, MWU,
// UICR, NVIC,

// Interrupt priorities reserved for SoftDevice
// Level 0: timing critical processing
// Level 1: memory isolation and run time protection handling
// Level 4: higher-level deferrable tasks and the API functions
// executed as SVC interrupts

// SoftDevice handler configuration
#define NRF_SDH_ENABLED                                       1
#define NRF_SDH_DISPATCH_MODEL                                0
#define NRF_SDH_CLOCK_LF_SRC                                  1
#define NRF_SDH_CLOCK_LF_RC_CTIV                              0
#define NRF_SDH_CLOCK_LF_RC_TEMP_CTIV                         0
#define NRF_SDH_CLOCK_LF_ACCURACY                             7
#define NRF_SDH_REQ_OBSERVER_PRIO_LEVELS                      2
#define NRF_SDH_STATE_OBSERVER_PRIO_LEVELS                    2
#define NRF_SDH_STACK_OBSERVER_PRIO_LEVELS                    2
#define CLOCK_CONFIG_STATE_OBSERVER_PRIO                      0
#define POWER_CONFIG_STATE_OBSERVER_PRIO                      0
#define RNG_CONFIG_STATE_OBSERVER_PRIO                        0
#define NRF_SDH_ANT_STACK_OBSERVER_PRIO                       0
#define NRF_SDH_BLE_STACK_OBSERVER_PRIO                       0
#define NRF_SDH_SOC_STACK_OBSERVER_PRIO                       0

// SoftDevice SoC event handler configuration
#define NRF_SDH_SOC_ENABLED                                   1
#define NRF_SDH_SOC_OBSERVER_PRIO_LEVELS                      2
#define BLE_DFU_SOC_OBSERVER_PRIO                             1
#define CLOCK_CONFIG_SOC_OBSERVER_PRIO                        0
#define POWER_CONFIG_SOC_OBSERVER_PRIO                        0

// SoftDevice BLE event handler configuration
#define NRF_SDH_BLE_ENABLED                                   1
#define NRF_SDH_BLE_GAP_DATA_LENGTH                           27
#define NRF_SDH_BLE_PERIPHERAL_LINK_COUNT                     1
#define NRF_SDH_BLE_CENTRAL_LINK_COUNT                        0
#define NRF_SDH_BLE_TOTAL_LINK_COUNT                          1
#define NRF_SDH_BLE_GAP_EVENT_LENGTH                          6
#define NRF_SDH_BLE_GATT_MAX_MTU_SIZE                         23
#define NRF_SDH_BLE_GATTS_ATTR_TAB_SIZE                       1408
#define NRF_SDH_BLE_VS_UUID_COUNT                             10
#define NRF_SDH_BLE_SERVICE_CHANGED                           0
#define NRF_SDH_BLE_OBSERVER_PRIO_LEVELS                      4
#define BLE_ADV_BLE_OBSERVER_PRIO                             1
#define BLE_ANCS_C_BLE_OBSERVER_PRIO                          2
#define BLE_ANS_C_BLE_OBSERVER_PRIO                           2
#define BLE_BAS_BLE_OBSERVER_PRIO                             2
#define BLE_BAS_C_BLE_OBSERVER_PRIO                           2
#define BLE_BPS_BLE_OBSERVER_PRIO                             2
#define BLE_CONN_PARAMS_BLE_OBSERVER_PRIO                     1
#define BLE_CONN_STATE_BLE_OBSERVER_PRIO                      0
#define BLE_CSCS_BLE_OBSERVER_PRIO                            2
#define BLE_CTS_C_BLE_OBSERVER_PRIO                           2
#define BLE_DB_DISC_BLE_OBSERVER_PRIO                         1
#define BLE_DFU_BLE_OBSERVER_PRIO                             2
#define BLE_DIS_C_BLE_OBSERVER_PRIO                           2
#define BLE_GLS_BLE_OBSERVER_PRIO                             2
#define BLE_HIDS_BLE_OBSERVER_PRIO                            2
#define BLE_HRS_BLE_OBSERVER_PRIO                             2
#define BLE_HRS_C_BLE_OBSERVER_PRIO                           2
#define BLE_HTS_BLE_OBSERVER_PRIO                             2
#define BLE_IAS_BLE_OBSERVER_PRIO                             2
#define BLE_IAS_C_BLE_OBSERVER_PRIO                           2
#define BLE_LBS_BLE_OBSERVER_PRIO                             2
#define BLE_LBS_C_BLE_OBSERVER_PRIO                           2
#define BLE_LLS_BLE_OBSERVER_PRIO                             2
#define BLE_LNS_BLE_OBSERVER_PRIO                             2
#define BLE_NUS_BLE_OBSERVER_PRIO                             2
#define BLE_NUS_C_BLE_OBSERVER_PRIO                           2
#define BLE_OTS_BLE_OBSERVER_PRIO                             2
#define BLE_OTS_C_BLE_OBSERVER_PRIO                           2
#define BLE_RSCS_BLE_OBSERVER_PRIO                            2
#define BLE_RSCS_C_BLE_OBSERVER_PRIO                          2
#define BLE_TPS_BLE_OBSERVER_PRIO                             2
#define BSP_BTN_BLE_OBSERVER_PRIO                             1
#define NFC_BLE_PAIR_LIB_BLE_OBSERVER_PRIO                    1
#define NRF_BLE_BMS_BLE_OBSERVER_PRIO                         2
#define NRF_BLE_CGMS_BLE_OBSERVER_PRIO                        2
#define NRF_BLE_ES_BLE_OBSERVER_PRIO                          2
#define NRF_BLE_GATTS_C_BLE_OBSERVER_PRIO                     2
#define NRF_BLE_GATT_BLE_OBSERVER_PRIO                        1
#define NRF_BLE_GQ_BLE_OBSERVER_PRIO                          1
#define NRF_BLE_QWR_BLE_OBSERVER_PRIO                         2
#define NRF_BLE_SCAN_OBSERVER_PRIO                            1
#define PM_BLE_OBSERVER_PRIO                                  1

// Connection parameters negotiation procedure configuration
#define NRF_BLE_CONN_PARAMS_ENABLED                           1
#define NRF_BLE_CONN_PARAMS_MAX_SLAVE_LATENCY_DEVIATION       499
#define NRF_BLE_CONN_PARAMS_MAX_SUPERVISION_TIMEOUT_DEVIATION 65535

// GATT module configuration
#define NRF_BLE_GATT_ENABLED                                  1
#define NRF_BLE_GATT_MTU_EXCHANGE_INITIATION_ENABLED          1

// Queued writes support module configuration
#define NRF_BLE_QWR_ENABLED                                   1
#define NRF_BLE_QWR_MAX_ATTR                                  0

// Advertising module configuration
#define BLE_ADVERTISING_ENABLED                               1

// Application timer functionality configuration
#define APP_TIMER_ENABLED                                     1
#define APP_TIMER_CONFIG_RTC_FREQUENCY                        1
#define APP_TIMER_CONFIG_IRQ_PRIORITY                         6
#define APP_TIMER_CONFIG_OP_QUEUE_SIZE                        10
#define APP_TIMER_CONFIG_USE_SCHEDULER                        0
#define APP_TIMER_KEEPS_RTC_ACTIVE                            0
#define APP_TIMER_SAFE_WINDOW_MS                              300000
#define APP_TIMER_WITH_PROFILER                               0
#define APP_TIMER_CONFIG_SWI_NUMBER                           0

// Power management module configuration
#define NRF_PWR_MGMT_ENABLED                                  1
#define NRF_PWR_MGMT_CONFIG_DEBUG_PIN_ENABLED                 0
#define NRF_PWR_MGMT_SLEEP_DEBUG_PIN                          31
#define NRF_PWR_MGMT_CONFIG_CPU_USAGE_MONITOR_ENABLED         0
#define NRF_PWR_MGMT_CONFIG_STANDBY_TIMEOUT_ENABLED           0
#define NRF_PWR_MGMT_CONFIG_STANDBY_TIMEOUT_S                 3
#define NRF_PWR_MGMT_CONFIG_FPU_SUPPORT_ENABLED               1
#define NRF_PWR_MGMT_CONFIG_AUTO_SHUTDOWN_RETRY               0
#define NRF_PWR_MGMT_CONFIG_USE_SCHEDULER                     0
#define NRF_PWR_MGMT_CONFIG_HANDLER_PRIORITY_COUNT            3

// Section iterator configuration
#define NRF_SECTION_ITER_ENABLED                              1

// logger frontend configuration
#define NRF_LOG_ENABLED                                       1
#define NRF_LOG_DEFAULT_LEVEL                                 4
#define NRF_LOG_USES_TIMESTAMP                                0
#define NRF_LOG_USES_COLORS                                   1
#define NRF_LOG_DEFERRED                                      1
#define NRF_LOG_ALLOW_OVERFLOW                                1
#define NRF_LOG_FILTERS_ENABLED                               0

// Logger configuration (additional)
#define NRF_LOG_MSGPOOL_ELEMENT_SIZE                          20
#define NRF_LOG_MSGPOOL_ELEMENT_COUNT                         8
#define NRF_LOG_BUFSIZE                                       1024
#define NRF_LOG_CLI_CMDS                                      0
#define NRF_LOG_NON_DEFFERED_CRITICAL_REGION_ENABLED          0
#define NRF_LOG_STR_PUSH_BUFFER_SIZE                          128
#define NRF_LOG_COLOR_DEFAULT                                 0
#define NRF_LOG_ERROR_COLOR                                   2
#define NRF_LOG_WARNING_COLOR                                 4
#define NRF_LOG_TIMESTAMP_DEFAULT_FREQUENCY                   0

// Log UART backend configuration
#define NRF_LOG_BACKEND_UART_ENABLED                          1
#define NRF_LOG_BACKEND_UART_TX_PIN                           (32 + 12)
// #define NRF_LOG_BACKEND_UART_BAUDRATE                  30801920
#define NRF_LOG_BACKEND_UART_BAUDRATE                         2576384
#define NRF_LOG_BACKEND_UART_TEMP_BUFFER_SIZE                 64

// UART/UARTE peripheral driver - legacy layer configuration
#define UART_ENABLED                                          1
#define UART_DEFAULT_CONFIG_HWFC                              0
#define UART_DEFAULT_CONFIG_PARITY                            0
// #define UART_DEFAULT_CONFIG_BAUDRATE                   30801920
#define UART_DEFAULT_CONFIG_BAUDRATE                          2576384
#define UART_DEFAULT_CONFIG_IRQ_PRIORITY                      6
#define UART_EASY_DMA_SUPPORT                                 1
#define UART_LEGACY_SUPPORT                                   1
#define UART0_ENABLED                                         1
#define UART0_CONFIG_USE_EASY_DMA                             1
#define UART1_ENABLED                                         0
#define UART1_CONFIG_USE_EASY_DMA                             0

// Peripheral Resource Sharing module configuration
#define NRFX_PRS_ENABLED                                      1
// #define NRFX_PRS_BOX_0_ENABLED
// #define NRFX_PRS_BOX_1_ENABLED
// #define NRFX_PRS_BOX_2_ENABLED
// #define NRFX_PRS_BOX_3_ENABLED
#define NRFX_PRS_BOX_4_ENABLED                                1
// #define NRFX_PRS_CONFIG_LOG_ENABLED
// #define NRFX_PRS_CONFIG_LOG_LEVEL
// #define NRFX_PRS_CONFIG_INFO_COLOR
// #define NRFX_PRS_CONFIG_DEBUG_COLOR

// Block allocator module configuration
#define NRF_BALLOC_ENABLED                                    1
#define NRF_BALLOC_CONFIG_DEBUG_ENABLED                       0
#define NRF_BALLOC_CONFIG_HEAD_GUARD_WORDS                    1
#define NRF_BALLOC_CONFIG_TAIL_GUARD_WORDS                    1
#define NRF_BALLOC_CONFIG_BASIC_CHECKS_ENABLED                0
#define NRF_BALLOC_CONFIG_DOUBLE_FREE_CHECK_ENABLED           0
#define NRF_BALLOC_CONFIG_DATA_TRASHING_CHECK_ENABLED         0
#define NRF_BALLOC_CLI_CMDS                                   0

// Linked memory allocator module configuration
#define NRF_MEMOBJ_ENABLED                                    1

// Library for converting error code to string configuration (doc in sdk 13)
#define NRF_STRERROR_ENABLED                                  1

// Log string formatter configuration
#define NRF_LOG_STR_FORMATTER_TIMESTAMP_FORMAT_ENABLED        1

// undocumented (?)
#define NRF_FPRINTF_ENABLED                                   1
#define NRF_FPRINTF_FLAG_AUTOMATIC_CR_ON_LF_ENABLED           1
#define NRF_FPRINTF_DOUBLE_ENABLED                            0

// GPIOTE peripheral driver configuration
#define NRFX_GPIOTE_ENABLED                                   1
#define NRFX_GPIOTE_CONFIG_NUM_OF_LOW_POWER_EVENTS            1
#define NRFX_GPIOTE_CONFIG_IRQ_PRIORITY                       6
// #define NRFX_GPIOTE_CONFIG_LOG_ENABLED
// #define NRFX_GPIOTE_CONFIG_LOG_LEVEL
// #define NRFX_GPIOTE_CONFIG_INFO_COLOR
// #define NRFX_GPIOTE_CONFIG_DEBUG_COLOR

// SAADC peripheral driver configuration
#define NRFX_SAADC_ENABLED                                    1
// #define NRFX_SAADC_CONFIG_RESOLUTION
// #define NRFX_SAADC_CONFIG_OVERSAMPLE
// #define NRFX_SAADC_CONFIG_LP_MODE
#define NRFX_SAADC_CONFIG_IRQ_PRIORITY                        6
// #define NRFX_SAADC_CONFIG_LOG_ENABLED 1
// #define NRFX_SAADC_CONFIG_LOG_LEVEL 4
// #define NRFX_SAADC_CONFIG_INFO_COLOR 0
// #define NRFX_SAADC_CONFIG_DEBUG_COLOR 0

// TIMER periperal driver configuration
#define NRFX_TIMER_ENABLED                                    1
#define NRFX_TIMER0_ENABLED                                   0
#define NRFX_TIMER1_ENABLED                                   1
#define NRFX_TIMER2_ENABLED                                   0
#define NRFX_TIMER3_ENABLED                                   0
#define NRFX_TIMER4_ENABLED                                   0
// #define 	NRFX_TIMER_DEFAULT_CONFIG_FREQUENCY
// #define 	NRFX_TIMER_DEFAULT_CONFIG_MODE
// #define 	NRFX_TIMER_DEFAULT_CONFIG_BIT_WIDTH
#define NRFX_TIMER_DEFAULT_CONFIG_IRQ_PRIORITY                6
// #define 	NRFX_TIMER_CONFIG_LOG_ENABLED 1
// #define 	NRFX_TIMER_CONFIG_LOG_LEVEL 4
// #define 	NRFX_TIMER_CONFIG_INFO_COLOR 0
// #define 	NRFX_TIMER_CONFIG_DEBUG_COLOR 0

// PWM peripheral driver configuration
#define NRFX_PWM_ENABLED                                      1
#define NRFX_PWM0_ENABLED                                     1
#define NRFX_PWM1_ENABLED                                     0
#define NRFX_PWM2_ENABLED                                     0
#define NRFX_PWM3_ENABLED                                     0
// #define NRFX_PWM_DEFAULT_CONFIG_OUT0_PIN
// #define NRFX_PWM_DEFAULT_CONFIG_OUT1_PIN
// #define NRFX_PWM_DEFAULT_CONFIG_OUT2_PIN
// #define NRFX_PWM_DEFAULT_CONFIG_OUT3_PIN
// #define NRFX_PWM_DEFAULT_CONFIG_BASE_CLOCK
// #define NRFX_PWM_DEFAULT_CONFIG_COUNT_MODE
// #define NRFX_PWM_DEFAULT_CONFIG_TOP_VALUE
// #define NRFX_PWM_DEFAULT_CONFIG_LOAD_MODE
// #define NRFX_PWM_DEFAULT_CONFIG_STEP_MODE
#define NRFX_PWM_DEFAULT_CONFIG_IRQ_PRIORITY                  6
// #define NRFX_PWM_CONFIG_LOG_ENABLED 1
// #define NRFX_PWM_CONFIG_LOG_LEVEL 4
// #define NRFX_PWM_CONFIG_INFO_COLOR 0
// #define NRFX_PWM_CONFIG_DEBUG_COLOR 0
// #define NRFX_PWM_NRF52_ANOMALY_109_WORKAROUND_ENABLED
// #define NRFX_PWM_NRF52_ANOMALY_109_EGU_INSTANCE

// PPI peripheral allocator configuration
#define NRFX_PPI_ENABLED                                      1
// #define NRFX_PPI_CONFIG_LOG_ENABLED 1
// #define NRFX_PPI_CONFIG_LOG_LEVEL 4
// #define NRFX_PPI_CONFIG_INFO_COLOR 0
// #define NRFX_PPI_CONFIG_DEBUG_COLOR 0

#endif  // SDK_CONFIG_H