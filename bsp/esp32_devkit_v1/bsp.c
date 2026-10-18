#include "bsp.h"

#include <stddef.h>

/** @brief A construction attempt reserves resources until firmware restart. */
static bool init_attempted;

static hal_status_t init_button(hal_gpio_t **button) {
    const hal_gpio_config_t config = {
        .pin = {.port = 0, .number = 18},
        .direction = HAL_GPIO_INPUT,
        .pull = HAL_GPIO_PULL_NONE, // External 10 kOhm pull-up.
    };
    return hal_gpio_config(button, &config);
}

static hal_status_t init_led(hal_gpio_t **led, bool active_level) {
    const hal_gpio_config_t config = {
        .pin = {.port = 0, .number = 19},
        .direction = HAL_GPIO_OUTPUT,
        .pull = HAL_GPIO_PULL_NONE,
        .initial_level = !active_level,
    };
    // Establish the inactive level before enabling the GPIO output.
    return hal_gpio_config(led, &config);
}

static hal_status_t init_i2c(hal_i2c_t **i2c) {
    const hal_i2c_config_t config = {
        .instance = 0,
        .clock_pin = {.port = 0, .number = 22},
        .data_pin = {.port = 0, .number = 21},
        .frequency = 100000,
        .timeout = 100,
    };
    return hal_i2c_config(i2c, &config);
}

static hal_status_t init_uart(hal_uart_t **uart) {
    const hal_uart_config_t config = {
        .instance = 0,
        .transmit_pin = {.port = 0, .number = 1},
        .receive_pin = {.port = 0, .number = 3},
        .baudrate = 115200,
        .data_width = HAL_UART_DATA_WIDTH_8,
        .parity = HAL_UART_PARITY_NONE,
        .stop_count = HAL_UART_STOP_COUNT_1,
        .timeout = 1000,
    };
    return hal_uart_config(uart, &config);
}

static hal_status_t init_timer(hal_timer_t **timer, hal_timer_callback_t callback) {
    const hal_timer_config_t config = {
        .instance = 0,
        .period = 1,
        .callback = callback,
    };
    return hal_timer_config(timer, &config);
}

hal_status_t bsp_init(hal_timer_callback_t timer_callback, bsp_resources_t *out_resources) {
    if (timer_callback == NULL || out_resources == NULL) {
        return HAL_STATUS_INVALID_ARGUMENT;
    }
    if (init_attempted) {
        return HAL_STATUS_INVALID_STATE;
    }
    init_attempted = true;

    bsp_resources_t resources = {
        .button_active_level = false,
        .led_active_level = true,
        .sht31_address = 0x44,
        .ssd1306_address = 0x3C,
    };

    hal_status_t status = init_led(&resources.led, resources.led_active_level);
    if (status == HAL_STATUS_OK) {
        status = init_button(&resources.button);
    }
    if (status == HAL_STATUS_OK) {
        status = init_i2c(&resources.devices_i2c);
    }
    if (status == HAL_STATUS_OK) {
        status = init_uart(&resources.telemetry_uart);
    }
    if (status == HAL_STATUS_OK) {
        status = init_timer(&resources.timebase_timer, timer_callback);
    }
    if (status == HAL_STATUS_OK) {
        // Publish the handles only after every resource has been constructed.
        *out_resources = resources;
    }
    return status;
}
