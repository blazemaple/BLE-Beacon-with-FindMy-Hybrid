#pragma once
#ifndef __DONG_PRINTER_H__
#define __DONG_PRINTER_H__

#include "nrf_log_ctrl.h"
#define MY_DEBUG 0
#if (MY_DEBUG)
#include "SEGGER_RTT.h"
#include "nrf_log.h"
#include "nrf_log_default_backends.h"
#endif

#if (MY_DEBUG)
//#define TAG "[MAIN] "
#define msg_print(fmt, ...)                       \
    do                                            \
    {                                             \
        NRF_LOG_RAW_INFO("[MAIN] " fmt, ##__VA_ARGS__); \
        NRF_LOG_FLUSH();                          \
    } while (0)
#define data_print(...)                \
    do                                 \
    {                                  \
        NRF_LOG_RAW_INFO(__VA_ARGS__); \
        NRF_LOG_FLUSH();               \
    } while (0)
#define rtt_printer(func) \
    do                    \
    {                     \
        func;             \
    } while (0)

/**@brief Function for initializing logging. */
void log_init(void);
#else
#define msg_print(fmt, ...) // do nothing
#define data_print(...)     // do nothing
#define rtt_printer(func)   // do nothing
#define log_init(void)      // empty_func() // do nothing

#endif
#endif // __DONG_PRINTER_H__