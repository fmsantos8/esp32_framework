/**
 * @file render.h
 * @author Fabricio Santos (fmsantos8@ucs.br)
 * @brief Internal GUI snapshots and framebuffer rendering.
 */
#ifndef RENDER_H
#define RENDER_H

#include "acquisition/app_acquisition.h"
#include "cmp_ssd1306.h"

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Screens selected by the application.
 *
 */
typedef enum {
    SCREEN_READINGS = 0, /** Temperature and humidity. */
    SCREEN_DIAGNOSTICS   /** Latest acquisition status and uptime. */
} gui_screen_t;

/**
 * @brief Immutable snapshot while its pages are being transferred.
 *
 */
typedef struct {
    acquisition_t acquisition; /** Latest acquisition and retained valid sample. */
    uint64_t uptime_seconds;   /** Time since startup, rounded down to seconds. */
    gui_screen_t screen;       /** Screen drawn for this snapshot. */
    bool has_result;           /** True after a completed acquisition attempt. */
} gui_frame_t;

/**
 * @brief Replace the framebuffer with the requested snapshot
 *
 * @param[in] display Display component.
 * @param[in] frame Required snapshot to draw.
 *
 * @retval SSD1306_STATUS_OK Framebuffer drawn.
 * @retval SSD1306_STATUS_INVALID_ARGUMENT Invalid display, sample, or drawing bounds.
 * @retval SSD1306_STATUS_NOT_INITIALIZED Display component is unavailable.
 * @retval SSD1306_STATUS_NOT_SUPPORTED A font or character is unavailable.
 */
ssd1306_status_t render_frame(ssd1306_t *display, const gui_frame_t *frame);

#endif
