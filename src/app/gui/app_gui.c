/**
 * @file app_gui.c
 * @author Fabricio Santos (fmsantos8@ucs.br)
 * @brief Coordinates screen selection, rendering, and display transfers.
 */
#include "app_gui.h"

#include <stddef.h>

#include "acquisition/app_acquisition.h"
#include "render.h"
#include "status_led/app_status_led.h"
#include "telemetry/app_telemetry.h"

#define RETRY_INTERVAL 1000U /** Delay after a failed display operation, in milliseconds. */

static uint32_t (*get_ticks)(void) = NULL;    /** Time source supplied by the integration. */
static gui_screen_t screen = SCREEN_READINGS; /** Requested screen. */
static gui_frame_t last_frame;                /** Snapshot currently drawn in the framebuffer. */
static uint64_t uptime = 0;                   /** Accumulated time since startup, in milliseconds. */
static uint32_t last_ticks = 0;               /** Last observed clock value. */
static uint32_t retry_since = 0;              /** Time after the latest failed operation returned. */
static bool pending = false;                  /** Pages remain in the current frame. */
static bool rendered = false;                 /** A complete snapshot has been drawn in RAM. */
static bool waiting_retry = false;            /** The next display attempt is delayed. */
static bool initialized = false;              /** True after successful task initialization. */

/**
 * @brief Check whether the requested snapshot differs from the rendered one.
 *
 * @param[in] frame Requested snapshot.
 *
 * @retval true Content must be redrawn.
 * @retval false The current framebuffer already represents the snapshot.
 */
static bool frame_changed(const gui_frame_t *frame) {
    if (!rendered || last_frame.screen != frame->screen || last_frame.has_result != frame->has_result) {
        return true;
    }
    if (frame->has_result &&
        (last_frame.acquisition.status != frame->acquisition.status ||
         last_frame.acquisition.has_sample != frame->acquisition.has_sample ||
         (frame->acquisition.has_sample &&
          (last_frame.acquisition.sample.temperature != frame->acquisition.sample.temperature ||
           last_frame.acquisition.sample.humidity != frame->acquisition.sample.humidity)))) {
        return true;
    }
    return frame->screen == SCREEN_DIAGNOSTICS && last_frame.uptime_seconds != frame->uptime_seconds;
}

bool app_gui_init(uint32_t (*get_ticks_function)(void)) {
    if (get_ticks_function == NULL) {
        return false;
    }
    get_ticks = get_ticks_function;
    last_ticks = get_ticks();
    uptime = last_ticks;
    screen = SCREEN_READINGS;
    last_frame = (gui_frame_t){0};
    retry_since = 0;
    pending = false;
    rendered = false;
    waiting_retry = false;
    initialized = true;
    return true;
}

void app_gui_toggle_screen(void) {
    if (initialized) {
        screen = screen == SCREEN_READINGS ? SCREEN_DIAGNOSTICS : SCREEN_READINGS;
    }
}

void app_gui_run(ssd1306_t *display) {
    if (display == NULL || !initialized) {
        return;
    }

    const uint32_t now = get_ticks();
    uptime += (uint32_t)(now - last_ticks);
    last_ticks = now;
    if (waiting_retry && (uint32_t)(now - retry_since) < RETRY_INTERVAL) {
        return;
    }

    ssd1306_status_t status = SSD1306_STATUS_OK;
    if (!pending) {
        gui_frame_t frame = {.screen = screen, .uptime_seconds = uptime / 1000U};
        frame.has_result = app_acquisition_get_result(&frame.acquisition);
        if (!frame_changed(&frame)) {
            return;
        }
        status = render_frame(display, &frame);
        if (status == SSD1306_STATUS_OK) {
            last_frame = frame;
            rendered = true;
            pending = true;
        } else {
            rendered = false;
        }
    }
    if (status == SSD1306_STATUS_OK) {
        status = cmp_ssd1306_update(display, &pending);
    }
    waiting_retry = status != SSD1306_STATUS_OK;
    if (waiting_retry) {
        retry_since = get_ticks();
        app_telemetry_report_error(APP_TELEMETRY_SOURCE_DISPLAY, (int32_t)status);
        if (status == SSD1306_STATUS_TIMEOUT || status == SSD1306_STATUS_COMMUNICATION_ERROR) {
            app_status_led_set_i2c_error(APP_STATUS_LED_I2C_DISPLAY, true);
        }
    } else {
        app_status_led_set_i2c_error(APP_STATUS_LED_I2C_DISPLAY, false);
    }
}
