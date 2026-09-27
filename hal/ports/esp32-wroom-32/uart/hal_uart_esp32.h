#ifndef HAL_UART_ESP32_H
#define HAL_UART_ESP32_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#include "driver/gpio.h"
#include "driver/uart.h"
#include "uart/hal_uart.h"

/**
 * @brief Concrete UART implementation for the ESP32-WROOM-32
 *
 */
struct hal_uart {
    const uart_port_t port;   /** UART controller number */
    const gpio_num_t tx_pin;  /** TX pin number */
    const gpio_num_t rx_pin;  /** RX pin number */
    uint32_t baud_rate;       /** Configured baud rate in bps */
    bool configured;          /** Whether the UART has been configured */
};

#ifdef __cplusplus
}
#endif

#endif
