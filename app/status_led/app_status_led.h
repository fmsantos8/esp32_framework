/**
 * @file app_status_led.h
 * @author Fabricio Santos (fmsantos8@ucs.br)
 * @brief Indicates startup, normal operation, and I2C communication errors.
 */
#ifndef APP_STATUS_LED_H
#define APP_STATUS_LED_H

#include "cmp_led.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Devices that can report an I2C communication error.
 */
typedef enum {
    APP_STATUS_LED_I2C_SENSOR = 0, /** SHT31-D communication. */
    APP_STATUS_LED_I2C_DISPLAY     /** SSD1306 communication. */
} app_status_led_i2c_source_t;

/**
 * @brief Initialize the task in startup mode without writing to the LED.
 *
 * @param[in] get_ticks_function Required monotonic time source in milliseconds, modulo 2^32.
 *
 * @retval true Task initialized successfully.
 * @retval false get_ticks_function is NULL; previous state is preserved.
 */
bool app_status_led_init(uint32_t (*get_ticks_function)(void));

/**
 * @brief Select normal operation or startup indication.
 *
 * @param[in] ready true for normal operation; false for startup. I2C errors take priority.
 */
void app_status_led_set_ready(bool ready);

/**
 * @brief Update one device's I2C error without clearing the other device's error.
 *
 * @param[in] source Sensor or display; invalid values are ignored.
 * @param[in] active true while communication has failed; false after recovery.
 */
void app_status_led_set_i2c_error(app_status_led_i2c_source_t source, bool active);

/**
 * @brief Keep the LED off at startup, on when ready, and blinking on I2C errors.
 *
 * @details Call sequentially for the same LED in normal context. Only changed
 * requests are written. Late calls perform at most one transition.
 *
 * @param[in] led LED component; NULL and calls before initialization are ignored.
 */
void app_status_led_run(const led_t *led);

#ifdef __cplusplus
}
#endif

#endif
