/**
 * @file hal_timer_esp32.h
 * @author Fabricio Santos (fmsantos8@ucs.br)
 * @brief Private elapsed-time representation.
 *
 */
#ifndef HAL_TIMER_ESP32_H
#define HAL_TIMER_ESP32_H

#include <stdbool.h>
#include <stdint.h>
#include "timer/hal_timer.h"

/**
 * @brief Snapshot of the system-clock origin for binding zero.
 */
struct hal_timer {
    int64_t origin;  /** Monotonic system-clock origin, in microseconds */
    bool configured; /** True after successful construction */
};

#endif
