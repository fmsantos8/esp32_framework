/**
 * @file app_telemetry.c
 * @author Fabricio Santos (fmsantos8@ucs.br)
 * @brief Buffers application records and handles UART retry and discard policies.
 */
#include "app_telemetry.h"
#include "app_telemetry_private.h"

#include <stddef.h>

static uint32_t (*get_ticks)(void) = NULL;     /** Time source supplied by the integration. */
static record_t records[QUEUE_CAPACITY] = {0}; /** Records queue */
static uint32_t head = 0;                      /** Oldest queued record. */
static uint32_t count = 0;                     /** Number of queued records. */
static uint32_t retry_since = 0;               /** Time after the latest failed send returned. */
static bool waiting_retry = false;             /** The next UART attempt is delayed. */
static bool initialized = false;               /** True after successful task initialization. */

static bool enqueue(const record_t *record) {
    if (!initialized) {
        return false;
    }
    if (count == QUEUE_CAPACITY) {
        return false;
    }
    records[(head + count) % QUEUE_CAPACITY] = *record;
    ++count;
    return true;
}

static void remove_head(void) {
    head = (head + 1U) % QUEUE_CAPACITY;
    --count;
}

bool app_telemetry_init(uint32_t (*get_ticks_function)(void)) {
    if (get_ticks_function == NULL) {
        return false;
    }
    get_ticks = get_ticks_function;
    head = 0;
    count = 0;
    retry_since = 0;
    waiting_retry = false;
    initialized = true;
    return true;
}

bool app_telemetry_report_sample(const telemetry_sample_t *sample) {
    if (sample == NULL) {
        return false;
    }
    const record_t record = {.kind = RECORD_SAMPLE, .sample = *sample};
    return enqueue(&record);
}

bool app_telemetry_report_error(telemetry_source_t source, int32_t code) {
    if (source != APP_TELEMETRY_SOURCE_SENSOR && source != APP_TELEMETRY_SOURCE_DISPLAY) {
        return false;
    }
    const record_t record = {.kind = RECORD_ERROR, .source = source, .code = code};
    return enqueue(&record);
}

void app_telemetry_run(const telemetry_t *telemetry) {
    if (telemetry == NULL || !initialized || count == 0) {
        return;
    }
    if (waiting_retry && (uint32_t)(get_ticks() - retry_since) < RETRY_INTERVAL) {
        return;
    }

    const record_t *record = &records[head];
    uint32_t transmitted = 0;
    telemetry_status_t status;
    if (record->kind == RECORD_SAMPLE) {
        status = cmp_telemetry_send_sample(telemetry, &record->sample, &transmitted);
    } else {
        const char *source = record->source == APP_TELEMETRY_SOURCE_SENSOR ? "SHT31D" : "SSD1306";
        status = cmp_telemetry_send_error(telemetry, source, record->code, &transmitted);
    }
    if (status == TELEMETRY_STATUS_OK) {
        remove_head();
        waiting_retry = false;
        return;
    }

    // Only retry transient failures that accepted no prefix of the record.
    if (transmitted != 0 || (status != TELEMETRY_STATUS_TIMEOUT && status != TELEMETRY_STATUS_COMMUNICATION_ERROR)) {
        remove_head();
    }
    retry_since = get_ticks();
    waiting_retry = true;
}
