#include <assert.h>
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

#ifdef NDEBUG
#error "This firmware requires assertions to be enabled"
#endif

#define HAL_STORAGE_CAPACITY 1024U /** HAL Memory Arena Storage (bytes) */

static _Alignas(max_align_t) unsigned char hal_storage[HAL_STORAGE_CAPACITY]; /** Shared HAL arena storage. */
static _Atomic uint32_t timer_notifications = 0;                              /** Serviced one-millisecond timer notifications. */

static bsp_resources_t board; /** Resources constructed by the selected board. */
static led_t led;             /** Status LED component context. */
static telemetry_t telemetry; /** UART telemetry component context. */
static button_t button;       /** Button component context. */
static sht31d_t sensor;       /** Sensor component context. */
static ssd1306_t display;     /** Display component context and framebuffer. */

_Static_assert(ATOMIC_INT_LOCK_FREE == 2, "Timer callback requires a lock-free counter");

static void IRAM_ATTR timer_tick(void) {
    atomic_fetch_add_explicit(&timer_notifications, 1, memory_order_relaxed);
}

static uint32_t get_ticks(void) {
    return atomic_load_explicit(&timer_notifications, memory_order_relaxed);
}

static void init_platform(void) {
    const hal_system_config_t system_config = {
        .memory_buffer = hal_storage,
        .memory_capacity = (uint32_t)sizeof(hal_storage),
    };
    assert(hal_system_init(&system_config) == HAL_STATUS_OK);

    assert(bsp_init(timer_tick, &board) == HAL_STATUS_OK);
    assert(hal_system_seal() == HAL_STATUS_OK);
}

static void init_components(void) {
    const led_config_t led_config = {
        .gpio = board.led,
        .active_level = board.led_active_level,
    };
    assert(cmp_led_init(&led, &led_config) == LED_STATUS_OK);

    const telemetry_config_t telemetry_config = {
        .uart = board.telemetry_uart,
    };
    assert(cmp_telemetry_init(&telemetry, &telemetry_config) == TELEMETRY_STATUS_OK);

    const button_config_t button_config = {
        .gpio = board.button,
        .active_level = board.button_active_level,
        .get_ticks = get_ticks,
    };
    assert(cmp_button_init(&button, &button_config) == BUTTON_STATUS_OK);

    const sht31d_config_t sensor_config = {
        .i2c = board.devices_i2c,
        .address = board.sht31_address,
    };
    assert(cmp_sht31d_init(&sensor, &sensor_config) == SHT31D_STATUS_OK);

    const ssd1306_config_t display_config = {
        .i2c = board.devices_i2c,
        .address = board.ssd1306_address,
    };
    assert(cmp_ssd1306_init(&display, &display_config) == SSD1306_STATUS_OK);
}

static void init_application(void) {
    assert(app_status_led_init(get_ticks));
    assert(app_telemetry_init(get_ticks));
    assert(app_acquisition_init(get_ticks));
    assert(app_gui_init(get_ticks));
    app_button_init();
}

void app_main(void) {
    init_platform();
    init_components();
    init_application();

    assert(hal_timer_start(board.timebase_timer) == HAL_STATUS_OK);

    app_status_led_set_ready(true);

    while (true) {
        app_button_run(&button);
        app_acquisition_run(&sensor);
        app_gui_run(&display);
        app_status_led_run(&led);
        app_telemetry_run(&telemetry);
        vTaskDelay(1);
    }
}
