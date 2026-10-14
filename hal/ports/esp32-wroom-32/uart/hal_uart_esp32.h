/**
 * @file hal_uart_esp32.h
 * @author Fabricio Santos (fmsantos8@ucs.br)
 * @brief Private UART representation.
 *
 */
#ifndef HAL_UART_ESP32_H
#define HAL_UART_ESP32_H

#include <stdbool.h>
#include "driver/uart.h"
#include "uart/hal_uart.h"

/**
 * @brief UART snapshot copied between typed local variables and arena bytes.
 */
struct hal_uart {
    uart_port_t controller;           /** SDK controller identifier */
    hal_uart_data_width_t data_width; /** Number of payload bits per character */
    bool configured;                  /** True after driver and routing initialization */
    uint32_t timeout;                 /** Configured transfer timeout, in milliseconds */
};

#endif
