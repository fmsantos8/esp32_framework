#ifndef HAL_GPIO_ESP32_H
#define HAL_GPIO_ESP32_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#include "driver/gpio.h"
#include "gpio/hal_gpio.h"

/**
 * @brief Concrete GPIO implementation for the ESP32-WROOM-32
 *
 */
struct hal_gpio {
    const gpio_num_t pin;           /** GPIO pin number */
    hal_gpio_direction_t direction; /** GPIO direction (input or output) */
    bool configured;                /** Whether the GPIO has been configured */
};

#ifdef __cplusplus
}
#endif

#endif
