/**
 * @file main.c
 * @author Fabricio Santos (fmsantos8@ucs.br)
 * @brief ESP32 composition and cooperative application scheduling.
 */
#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "app.h"
#include "app_config.h"
#include "bsp.h"
#include "cmp_button_config.h"
#include "cmp_led_config.h"
#include "cmp_sht31d_config.h"
#include "cmp_ssd1306_config.h"
#include "cmp_telemetry_config.h"
#include "esp_attr.h"
#include "system/hal_system.h"

/** @brief Persistent storage for every HAL resource. */
_Alignas(max_align_t) static unsigned char hal_storage[1024];

/** @brief Board-selected resources, shared only by the injected components. */
static bsp_resources_t board;

/** @brief Serviced 1 ms notifications; interrupt delays can undercount wall time. */
static _Atomic uint32_t timer_notifications;

/** @brief Fixed component and application storage, independent of the HAL arena. */
static sht31d_t sensor;
static ssd1306_t display;
static button_t button;
static led_t led;
static telemetry_t telemetry;
static app_t application;

/** @brief Sensor and display configurations retained for application recovery. */
static sht31d_config_t sensor_config;
static ssd1306_config_t display_config;

_Static_assert(ATOMIC_INT_LOCK_FREE == 2, "Timer callback requires lock-free atomic counter");

static void IRAM_ATTR timer_tick(void) {
    atomic_fetch_add_explicit(&timer_notifications, 1, memory_order_relaxed);
}

static uint32_t get_ticks(void) {
    return atomic_load_explicit(&timer_notifications, memory_order_relaxed);
}

static void report_startup_error(const char *source, int32_t code) {
    uint32_t transmitted;
    cmp_telemetry_send_error(&telemetry, source, code, &transmitted);
}

void app_main(void) {
    const hal_system_config_t system_config = {
        .memory_buffer = hal_storage,
        .memory_capacity = sizeof(hal_storage),
    };
    if (hal_system_init(&system_config) != HAL_STATUS_OK || bsp_init(timer_tick, &board) != HAL_STATUS_OK ||
        hal_system_seal() != HAL_STATUS_OK) {
        return;
    }

    const telemetry_config_t telemetry_config = {.uart = board.telemetry_uart};
    if (cmp_telemetry_init(&telemetry, &telemetry_config) != TELEMETRY_STATUS_OK) {
        return;
    }
    const led_config_t led_config = {.gpio = board.led, .active_level = board.led_active_level};
    led_status_t led_status = cmp_led_init(&led, &led_config);
    if (led_status != LED_STATUS_OK) {
        report_startup_error("led_init", led_status);
        return;
    }
    const button_config_t button_config = {
        .gpio = board.button,
        .active_level = board.button_active_level,
        .debounce = CMP_BUTTON_DEFAULT_DEBOUNCE,
        .get_ticks = get_ticks,
    };
    button_status_t button_status = cmp_button_init(&button, &button_config);
    if (button_status != BUTTON_STATUS_OK) {
        report_startup_error("button_init", button_status);
        return;
    }
    sensor_config = (sht31d_config_t){.i2c = board.devices_i2c, .address = board.sht31_address};
    display_config = (ssd1306_config_t){.i2c = board.devices_i2c, .address = board.ssd1306_address};
    const app_config_t app_config = {
        .sensor = &sensor,
        .sensor_config = &sensor_config,
        .display = &display,
        .display_config = &display_config,
        .button = &button,
        .led = &led,
        .telemetry = &telemetry,
        .get_ticks = get_ticks,
    };
    app_status_t app_status = app_init(&application, &app_config);
    if (app_status != APP_STATUS_OK) {
        report_startup_error("app_init", app_status);
        return;
    }
    hal_status_t timer_status = hal_timer_start(board.timebase_timer);
    if (timer_status != HAL_STATUS_OK) {
        report_startup_error("timer_start", timer_status);
        return;
    }

    // The platform chooses how often the portable modules get processing time.
    TickType_t delay = pdMS_TO_TICKS(5);
    if (delay == 0) {
        delay = 1;
    }
    for (;;) {
        app_process(&application);
        vTaskDelay(delay);
    }
}
