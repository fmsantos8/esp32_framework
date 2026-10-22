/**
 * @file app_telemetry_types.h
 * @author Fabricio Santos (fmsantos8@ucs.br)
 * @brief Bounded telemetry queue and retry state.
 */
#ifndef APP_TELEMETRY_TYPES_H
#define APP_TELEMETRY_TYPES_H

#include "app_types.h"

#define APP_TELEMETRY_ERROR_CAPACITY 8U /** Maximum queued error records. */

/** @brief An application error awaiting transmission. */
typedef struct {
    app_error_source_t source; /** Origin mapped to a fixed telemetry identifier. */
    int32_t code;              /** Component-specific status code. */
} app_telemetry_error_t;

/** @brief Borrowed UART component and positive retry interval. */
typedef struct {
    telemetry_t *telemetry;    /** Initialized component. */
    app_get_ticks_t get_ticks; /** Coordinator's millisecond time source. */
    uint32_t retry;            /** Delay after a failed record, at most INT32_MAX. */
} app_telemetry_config_t;

/** @brief Caller-owned storage; errors are FIFO and samples keep only the newest. */
typedef struct {
    app_telemetry_config_t config;                              /** Borrowed dependencies. */
    app_telemetry_error_t errors[APP_TELEMETRY_ERROR_CAPACITY]; /** Pending error records. */
    telemetry_sample_t sample;                                  /** Newest unsent sample. */
    uint32_t head;                                              /** First queued error index. */
    uint32_t count;                                             /** Number of queued errors. */
    uint32_t dropped_errors;                                    /** Saturating count of rejected errors. */
    uint32_t retry_since;                                       /** Timestamp after a failed send. */
    bool sample_pending;                                        /** The sample awaits one send attempt. */
    bool waiting_retry;                                         /** Further attempts are delayed. */
    bool initialized;                                           /** Context is available. */
} app_telemetry_t;

#endif
