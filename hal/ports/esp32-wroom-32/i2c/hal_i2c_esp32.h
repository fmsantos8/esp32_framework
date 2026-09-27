#ifndef HAL_I2C_ESP32_H
#define HAL_I2C_ESP32_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "i2c/hal_i2c.h"

/**
 * @brief Concrete I2C implementation for the ESP32-WROOM-32
 *
 */
struct hal_i2c {
    const i2c_port_num_t controller; /** I2C controller number */
    const gpio_num_t sda_pin;        /** SDA pin number */
    const gpio_num_t scl_pin;        /** SCL pin number */
    i2c_master_bus_handle_t bus;     /** ESP-IDF bus handle, managed by this port */
    uint32_t clock_speed;            /** Configured clock speed in Hz */
};

#ifdef __cplusplus
}
#endif

#endif
