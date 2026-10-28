/**
 * @file app_telemetry_private.h
 * @author Fabricio Santos (fmsantos8@ucs.br)
 * @brief Private
 */
#ifndef APP_TELEMETRY_PRIVATE_H
#define APP_TELEMETRY_PRIVATE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#include "cmp_telemetry_types.h"

#define QUEUE_CAPACITY 8U    /** Maximum number of queued records. */
#define RETRY_INTERVAL 1000U /** Delay after a failed send, in milliseconds. */

/**
 * @brief Records accepted by the application queue.
 */
typedef enum {
    RECORD_SAMPLE = 0, /** One valid sensor measurement. */
    RECORD_ERROR       /** One component error occurrence. */
} record_kind_t;

/**
 * @brief Owned copy of one reading or error occurrence.
 */
typedef struct {
    record_kind_t kind;        /** Selected payload. */
    telemetry_sample_t sample; /** Reading to transmit. */
    telemetry_source_t source; /** Device that produced the error code. */
    int32_t code;              /** Original component result. */
} record_t;

#ifdef __cplusplus
}
#endif

#endif
