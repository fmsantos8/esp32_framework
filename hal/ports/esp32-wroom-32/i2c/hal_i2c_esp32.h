/**
 * @file hal_i2c_esp32.h
 * @author Fabricio Santos (fmsantos8@ucs.br)
 * @brief Private shared-bus I2C representation.
 *
 */
#ifndef HAL_I2C_ESP32_H
#define HAL_I2C_ESP32_H

#include "driver/i2c_master.h"
#include "i2c/hal_i2c.h"

/**
 * @brief I2C snapshot copied to and from arena byte storage.
 */
struct hal_i2c {
    i2c_master_bus_handle_t bus;    /** SDK bus owned by this port */
    i2c_master_dev_handle_t device; /** Address-free SDK device allocated once during construction */
    uint32_t timeout;               /** Configured transfer timeout, in milliseconds */
};

#endif
