/**
 * @file hal_gpio_esp32.h
 * @author Fabricio Santos (fmsantos8@ucs.br)
 * @brief Private GPIO representation copied to and from arena storage.
 *
 */
#ifndef HAL_GPIO_ESP32_H
#define HAL_GPIO_ESP32_H

#include "driver/gpio.h"
#include "gpio/hal_gpio.h"

/**
 * @brief Private GPIO snapshot; arena handles are never dereferenced as this type.
 */
struct hal_gpio {
    gpio_num_t pin;                 /** Physical GPIO index */
    hal_gpio_direction_t direction; /** Configured digital direction */
    bool configured;                /** True after successful construction */
};

#endif
