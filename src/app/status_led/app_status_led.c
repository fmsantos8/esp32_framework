/**
 * @file app_status_led.c
 * @author Fabricio Santos (fmsantos8@ucs.br)
 * @brief Indicates startup, normal operation, and I2C communication errors.
 */
#include "app_status_led.h"

#include <stddef.h>

#define APP_STATUS_LED_BLINK_INTERVAL 500U /** Time between blink transitions, in milliseconds. */

static uint32_t (*get_ticks)(void) = NULL; /** Time source supplied by the integration. */
static uint8_t i2c_errors = 0;             /** Independent sensor and display error bits. */
static bool operation_ready = false;       /** True after startup is complete. */
static uint32_t last_transition = 0;       /** Timestamp of the current blink phase. */
static int8_t led_state = -1;              /** Last requested led state; -1 before the first command. */
static bool blink_active = false;          /** True after starting the current blink sequence. */
static bool initialized = false;           /** True after successful task initialization. */

bool app_status_led_init(uint32_t (*get_ticks_function)(void)) {
    if (get_ticks_function == NULL) {
        return false;
    }

    get_ticks = get_ticks_function;
    last_transition = 0;
    i2c_errors = 0;
    operation_ready = false;
    led_state = -1;
    blink_active = false;
    initialized = true;
    return true;
}

void app_status_led_set_ready(bool ready) {
    if (initialized) {
        operation_ready = ready;
    }
}

void app_status_led_set_i2c_error(app_status_led_i2c_source_t source, bool active) {
    if (!initialized || (source != APP_STATUS_LED_I2C_SENSOR && source != APP_STATUS_LED_I2C_DISPLAY)) {
        return;
    }

    const uint8_t bit = (uint8_t)(1U << (uint8_t)source);
    if (active) {
        i2c_errors |= bit;
    } else {
        i2c_errors &= (uint8_t)~bit;
    }
}

void app_status_led_run(const led_t *led) {
    if (led == NULL || !initialized) {
        return;
    }

    bool state_to_set = operation_ready;

    if (i2c_errors != 0) {
        const uint32_t now = get_ticks();
        state_to_set = led_state == 1;

        if (!blink_active) {
            blink_active = true;
            state_to_set = false;
            last_transition = now;
        } else if ((uint32_t)(now - last_transition) >= APP_STATUS_LED_BLINK_INTERVAL) {
            state_to_set = !state_to_set;
            last_transition = now;
        }
    } else {
        blink_active = false;
    }

    if (state_to_set != led_state) {
        cmp_led_set(led, state_to_set);
        led_state = state_to_set;
    }
}
