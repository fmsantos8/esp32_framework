#include "bsp.h"

#include <assert.h>
#include <stddef.h>

#define HAL_STORAGE_CAPACITY 1024U /** HAL Memory Arena Storage (bytes) */

static bool init_attempted = false;
static _Alignas(max_align_t) unsigned char hal_storage[HAL_STORAGE_CAPACITY]; /** Shared HAL arena storage. */

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

static void init_system(void) {
    const hal_system_config_t system_config = {
        .memory_buffer = hal_storage,
        .memory_capacity = (uint32_t)sizeof(hal_storage),
        .cpu_clock = 160000000U, // 160 MHz CPU clock frequency
    };
    assert(hal_system_init(&system_config) == HAL_STATUS_OK);
}

hal_status_t bsp_init(hal_timer_callback_t timer_callback, bsp_resources_t *resources) {

    if (timer_callback == NULL || resources == NULL) {
        return HAL_STATUS_INVALID_ARGUMENT;
    }
    if (init_attempted) {
        return HAL_STATUS_INVALID_STATE;
    }
    init_attempted = true;

    init_system();

    bsp_resources_t configured_resources = {
        .button_active_level = false,
        .led_active_level = true,
        .sht31_address = 0x44,
        .ssd1306_address = 0x3C,
    };

    hal_status_t status = init_led(&configured_resources.led, configured_resources.led_active_level);
    if (status == HAL_STATUS_OK) {
        status = init_button(&configured_resources.button);
    }
    if (status == HAL_STATUS_OK) {
        status = init_i2c(&configured_resources.devices_i2c);
    }
    if (status == HAL_STATUS_OK) {
        status = init_uart(&configured_resources.telemetry_uart);
    }
    if (status == HAL_STATUS_OK) {
        status = init_timer(&configured_resources.timebase_timer, timer_callback);
    }
    if (status == HAL_STATUS_OK) {
        // Publish the handles only after every resource has been constructed.
        *resources = configured_resources;
    }

    assert(hal_system_seal() == HAL_STATUS_OK);

    return status;
}
