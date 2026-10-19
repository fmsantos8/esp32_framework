/**
 * @file ssd1306_config.h
 * @brief Construction and static storage for the SSD1306 composition root.
 */
#ifndef SSD1306_CONFIG_H
#define SSD1306_CONFIG_H

#include "ssd1306.h"

#include "i2c/hal_i2c.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Dependencies for a 128x64 module with an internal charge pump.
 * @details The board supplies stable power, a completed hardware reset, and I2C
 * pull-ups. The composition supplies a constructed bus and the device address;
 * this component does not select pins, configure the bus, or own its handle.
 */
struct ssd1306_config {
    hal_i2c_t *i2c;  /** Required, already constructed I2C master bus. */
    uint8_t address; /** Unshifted SSD1306 address: 0x3C or 0x3D. */
};

/**
 * @brief Caller-owned persistent storage; fields are private to the component.
 * @details Declare this in the composition root and pass ssd1306_t pointers to
 * consumers. No singleton or HAL arena allocation is used. A static instance is
 * recommended; init() supplies all initial values, even for automatic storage.
 */
struct ssd1306 {
    hal_i2c_t *i2c;                                /** Borrowed bus handle. */
    uint8_t address;                               /** Borrowed device address. */
    uint8_t framebuffer[SSD1306_FRAMEBUFFER_SIZE]; /** Page-major pixels, bit zero at the top of each page. */
    uint8_t dirty_pages;                           /** One bit per page requiring transfer. */
    bool initialized;                              /** True only after a complete successful initialization. */
};

#ifdef __cplusplus
}
#endif

#endif
