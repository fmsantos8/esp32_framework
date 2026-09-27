#ifndef HAL_TIMER_ESP32_H
#define HAL_TIMER_ESP32_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include "driver/gptimer.h"
#include "timer/hal_timer.h"

/**
 * @brief Concrete timer implementation for the ESP32-WROOM-32
 *
 */
struct hal_timer {
    gptimer_handle_t driver; /** Timer handle */
    bool started;            /** Whether the timer has started */
};

#ifdef __cplusplus
}
#endif

#endif
