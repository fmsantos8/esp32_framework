/**
 * @file app_indication_types.h
 * @author Fabricio Santos (fmsantos8@ucs.br)
 * @brief LED indication dependencies and state.
 */
#ifndef APP_INDICATION_TYPES_H
#define APP_INDICATION_TYPES_H

#include "app_types.h"

/** @brief Product states represented by the LED. */
typedef enum {
    APP_INDICATION_STARTUP = 0, /** Off until startup produces an acquisition result. */
    APP_INDICATION_HEALTHY,     /** Continuously on. */
    APP_INDICATION_FAULT        /** Alternates at the configured interval. */
} app_indication_mode_t;

/** @brief Borrowed LED and timing parameters. */
typedef struct {
    led_t *led;                /** Initialized LED, initially off. */
    app_get_ticks_t get_ticks; /** Coordinator's millisecond time source. */
    uint32_t blink_period;     /** Positive interval between fault LED transitions. */
    uint32_t retry;            /** Positive interval after an output failure. */
} app_indication_config_t;

/** @brief Caller-owned LED policy state. */
typedef struct {
    app_indication_config_t config; /** Borrowed dependencies and intervals. */
    uint32_t phase_since;           /** Timestamp of the most recent blink transition. */
    uint32_t retry_since;           /** Timestamp after a failed output returned. */
    app_indication_mode_t mode;     /** Current indication mode. */
    bool phase_on;                  /** Logical level of the current blink phase. */
    bool level;                     /** Last successfully applied logical level. */
    bool applied;                   /** The cached level is valid. */
    bool waiting_retry;             /** An output retry is delayed. */
    bool initialized;               /** Context is available. */
} app_indication_t;

#endif
