/**
 * @file main.c
 * @author Fabricio Santos (fmsantos8@ucs.br)
 * @brief Composes the ESP32 firmware and runs its applications sequentially.
 */
#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_attr.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "acquisition/app_acquisition.h"
#include "bsp.h"
#include "button/app_button.h"
#include "cmp_button_config.h"
#include "cmp_led_config.h"
#include "cmp_sht31d_config.h"
#include "cmp_ssd1306_config.h"
#include "cmp_telemetry_config.h"
#include "gui/app_gui.h"
#include "status_led/app_status_led.h"
#include "system/hal_system.h"
#include "telemetry/app_telemetry.h"

#define HAL_STORAGE_CAPACITY 1024U /** Persistent HAL object storage, in bytes. */

static _Alignas(max_align_t) unsigned char hal_storage[HAL_STORAGE_CAPACITY]; /** Shared HAL arena storage. */
static _Atomic uint32_t timer_notifications = 0; /** Serviced one-millisecond timer notifications. */

_Static_assert(ATOMIC_INT_LOCK_FREE == 2, "Timer callback requires a lock-free counter");

static void IRAM_ATTR timer_tick(void) {
    atomic_fetch_add_explicit(&timer_notifications, 1, memory_order_relaxed);
}

static uint32_t get_ticks(void) {
    return atomic_load_explicit(&timer_notifications, memory_order_relaxed);
}

/** @brief Keep startup failure outputs active until the firmware is restarted. */
static _Noreturn void wait_for_restart(const led_t *led, const telemetry_t *telemetry) {
    while (true) {
        app_status_led_run(led);
        app_telemetry_run(telemetry);
        vTaskDelay(1);
    }
}

/** @brief Initialize the board and components, then run the cooperative application loop. */
void app_main(void) {
    static bsp_resources_t board;
    static button_t button;
    static led_t led;
    static sht31d_t sensor;
    static ssd1306_t display;
    static telemetry_t telemetry;

    const hal_system_config_t system_config = {
        .memory_buffer = hal_storage,
        .memory_capacity = (uint32_t)sizeof(hal_storage),
    };
    if (hal_system_init(&system_config) != HAL_STATUS_OK ||
        bsp_init(timer_tick, &board) != HAL_STATUS_OK ||
        hal_system_seal() != HAL_STATUS_OK) {
        return;
    }

    const led_config_t led_config = {
        .gpio = board.led,
        .active_level = board.led_active_level,
    };
    const telemetry_config_t telemetry_config = {
        .uart = board.telemetry_uart,
    };
    const button_config_t button_config = {
        .gpio = board.button,
        .active_level = board.button_active_level,
        .get_ticks = get_ticks,
    };
    if (cmp_led_init(&led, &led_config) != LED_STATUS_OK ||
        cmp_telemetry_init(&telemetry, &telemetry_config) != TELEMETRY_STATUS_OK ||
        cmp_button_init(&button, &button_config) != BUTTON_STATUS_OK) {
        return;
    }

    if (!app_status_led_init(get_ticks) ||
        !app_telemetry_init(get_ticks) ||
        !app_acquisition_init(get_ticks) ||
        !app_gui_init(get_ticks)) {
        return;
    }
    app_button_init();
    app_status_led_run(&led);

    if (hal_timer_start(board.timebase_timer) != HAL_STATUS_OK) {
        return;
    }

    const sht31d_config_t sensor_config = {
        .i2c = board.devices_i2c,
        .address = board.sht31_address,
    };
    const sht31d_status_t sensor_status = cmp_sht31d_init(&sensor, &sensor_config);
    if (sensor_status != SHT31D_STATUS_OK) {
        app_status_led_set_i2c_error(APP_STATUS_LED_I2C_SENSOR,
                                    sensor_status == SHT31D_STATUS_TIMEOUT || sensor_status == SHT31D_STATUS_COMMUNICATION_ERROR);
        app_telemetry_report_error(APP_TELEMETRY_SOURCE_SENSOR, (int32_t)sensor_status);
        wait_for_restart(&led, &telemetry);
    }

    const ssd1306_config_t display_config = {
        .i2c = board.devices_i2c,
        .address = board.ssd1306_address,
    };
    const ssd1306_status_t display_status = cmp_ssd1306_init(&display, &display_config);
    if (display_status != SSD1306_STATUS_OK) {
        app_status_led_set_i2c_error(APP_STATUS_LED_I2C_DISPLAY,
                                    display_status == SSD1306_STATUS_TIMEOUT || display_status == SSD1306_STATUS_COMMUNICATION_ERROR);
        app_telemetry_report_error(APP_TELEMETRY_SOURCE_DISPLAY, (int32_t)display_status);
        wait_for_restart(&led, &telemetry);
    }

    app_status_led_set_ready(true);
    app_status_led_run(&led);

    while (true) {
        app_button_run(&button);
        app_acquisition_run(&sensor);
        app_gui_run(&display);
        app_status_led_run(&led);
        app_telemetry_run(&telemetry);
        vTaskDelay(1);
    }
}
