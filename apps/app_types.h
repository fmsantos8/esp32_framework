/**
 * @file app_types.h
 * @author Fabricio Santos (fmsantos8@ucs.br)
 * @brief Shared application state and portable contracts.
 */
#ifndef APP_TYPES_H
#define APP_TYPES_H

#include <stdbool.h>
#include <stdint.h>

#include "cmp_button_types.h"
#include "cmp_led_types.h"
#include "cmp_sht31d.h"
#include "cmp_ssd1306_types.h"
#include "cmp_telemetry_types.h"

/** @brief Caller-owned coordinator, defined in app_config.h. */
typedef struct app app_t;

/** @brief Borrowed dependencies and intervals, defined in app_config.h. */
typedef struct app_config app_config_t;

/**
 * @brief Read a monotonic millisecond counter modulo 2^32.
 *
 * @details Injected by composition; must return promptly in normal context.
 * Synchronize interrupt-updated storage in the provider. Call app_process()
 * at least once per counter wrap; elapsed time has the provider's accuracy.
 *
 * @return Current counter value.
 */
typedef uint32_t (*app_get_ticks_t)(void);

/** @brief Application-level results. */
typedef enum {
    APP_STATUS_OK = 0,           /** Operation completed. */
    APP_STATUS_INVALID_ARGUMENT, /** Invalid dependency or interval. */
    APP_STATUS_NOT_INITIALIZED,  /** Context has not been initialized. */
    APP_STATUS_COMPONENT_ERROR,  /** A component reports a fault; processing continues. */
    APP_STATUS_QUEUE_FULL        /** The new telemetry error was dropped. */
} app_status_t;

/** @brief Screens selected by a debounced button press. */
typedef enum {
    APP_SCREEN_READINGS = 0, /** Temperature and relative humidity. */
    APP_SCREEN_DIAGNOSTICS   /** Last acquisition result and uptime. */
} app_screen_t;

/** @brief Sources accepted by the telemetry error queue. */
typedef enum {
    APP_ERROR_ACQUISITION = 0, /** Sensor acquisition. */
    APP_ERROR_BUTTON,          /** Button input. */
    APP_ERROR_INTERFACE,       /** Display output. */
    APP_ERROR_INDICATION       /** LED output. */
} app_error_source_t;

/** @brief Application snapshot; the sample survives failed acquisitions. */
typedef struct {
    sht31d_sample_t sample;              /** Last valid sample; meaningful when has_sample. */
    bool has_sample;                     /** A valid sample has been acquired. */
    bool acquisition_attempted;          /** At least one acquisition result is available. */
    uint32_t acquisition_revision;       /** Changes after each sample or acquisition failure. */
    sht31d_status_t acquisition_status;  /** Most recent acquisition result. */
    button_status_t button_status;       /** Most recent button result. */
    ssd1306_status_t interface_status;   /** Most recent display result. */
    led_status_t indication_status;      /** Most recent LED result. */
    telemetry_status_t telemetry_status; /** Most recent UART record result. */
    uint32_t dropped_errors;             /** Saturating count of errors rejected by the full queue. */
    app_screen_t screen;                 /** Selected screen. */
    uint64_t uptime;                     /** Accumulated milliseconds since app_init(). */
    bool started;                        /** Acquisition and display startup have been attempted. */
    bool fault;                          /** At least one component reports a fault. */
} app_state_t;

#endif
