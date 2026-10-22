/**
 * @file app_config.h
 * @author Fabricio Santos (fmsantos8@ucs.br)
 * @brief Application composition and caller-owned coordinator storage.
 */
#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include "acquisition/app_acquisition_types.h"
#include "button/app_button_types.h"
#include "indication/app_indication_types.h"
#include "interface/app_interface_types.h"
#include "telemetry/app_telemetry_types.h"

#define APP_DEFAULT_ACQUISITION_PERIOD 2000U /** Milliseconds between measurement starts. */
#define APP_DEFAULT_RETRY_PERIOD       2000U /** Milliseconds before component recovery. */
#define APP_DEFAULT_BLINK_PERIOD       500U  /** Milliseconds between fault LED transitions. */

/** @brief Construction dependencies, borrowed for the complete application lifetime. */
struct app_config {
    sht31d_t *sensor;                       /** Sensor storage; application initializes it. */
    const sht31d_config_t *sensor_config;   /** Persistent sensor configuration, opaque to apps. */
    ssd1306_t *display;                     /** Display storage; application initializes it. */
    const ssd1306_config_t *display_config; /** Persistent display configuration, opaque to apps. */
    button_t *button;                       /** Initialized debounced button. */
    led_t *led;                             /** Initialized LED, initially off. */
    telemetry_t *telemetry;                 /** Initialized telemetry output. */
    app_get_ticks_t get_ticks;              /** Millisecond source also used by the button. */
    uint32_t acquisition_period;            /** Zero selects default; otherwise 16..INT32_MAX ms. */
    uint32_t retry_period;                  /** Zero selects default; otherwise 16..INT32_MAX ms. */
    uint32_t blink_period;                  /** Zero selects default; otherwise 1..INT32_MAX ms. */
};

/** @brief Fixed storage for all cooperative application modules; no dynamic allocation. */
struct app {
    app_acquisition_t acquisition; /** Acquisition state machine. */
    app_button_t button;           /** Press tracking and screen selection. */
    app_interface_t interface;     /** Rendering and panel recovery. */
    app_indication_t indication;   /** LED state and output recovery. */
    app_telemetry_t telemetry;     /** Bounded pending records. */
    app_state_t state;             /** Latest observable product state. */
    app_get_ticks_t get_ticks;     /** Shared time source. */
    uint32_t last_ticks;           /** Last uptime accumulation timestamp. */
    bool initialized;              /** Coordinator is available. */
};

#endif
