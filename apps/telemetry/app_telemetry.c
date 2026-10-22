/**
 * @file app_telemetry.c
 * @author Fabricio Santos (fmsantos8@ucs.br)
 * @brief Bounded telemetry scheduling.
 */
#include "app_telemetry.h"

#include <limits.h>
#include <stddef.h>

#include "cmp_telemetry.h"

/** @brief Identifiers accepted by the component's wire format. */
static const char *const app_telemetry_sources[] = {"acquisition", "button", "interface", "indication"};

app_status_t app_telemetry_init(app_telemetry_t *telemetry, const app_telemetry_config_t *config) {
    if (telemetry == NULL || config == NULL || config->telemetry == NULL || config->get_ticks == NULL ||
        config->retry == 0 || config->retry > INT32_MAX) {
        return APP_STATUS_INVALID_ARGUMENT;
    }
    *telemetry = (app_telemetry_t){.config = *config, .initialized = true};
    return APP_STATUS_OK;
}

app_status_t app_telemetry_queue_sample(app_telemetry_t *telemetry, const sht31d_sample_t *sample) {
    if (telemetry == NULL || sample == NULL) {
        return APP_STATUS_INVALID_ARGUMENT;
    }
    if (!telemetry->initialized) {
        return APP_STATUS_NOT_INITIALIZED;
    }
    telemetry->sample = (telemetry_sample_t){.temperature = sample->temperature, .humidity = sample->humidity};
    telemetry->sample_pending = true;
    return APP_STATUS_OK;
}

app_status_t app_telemetry_queue_error(app_telemetry_t *telemetry, app_error_source_t source, int32_t code) {
    if (telemetry == NULL || source < APP_ERROR_ACQUISITION || source > APP_ERROR_INDICATION) {
        return APP_STATUS_INVALID_ARGUMENT;
    }
    if (!telemetry->initialized) {
        return APP_STATUS_NOT_INITIALIZED;
    }
    if (telemetry->count == APP_TELEMETRY_ERROR_CAPACITY) {
        if (telemetry->dropped_errors != UINT32_MAX) {
            ++telemetry->dropped_errors;
        }
        return APP_STATUS_QUEUE_FULL;
    }
    uint32_t tail = (telemetry->head + telemetry->count) % APP_TELEMETRY_ERROR_CAPACITY;
    telemetry->errors[tail] = (app_telemetry_error_t){.source = source, .code = code};
    ++telemetry->count;
    return APP_STATUS_OK;
}

telemetry_status_t app_telemetry_process(app_telemetry_t *telemetry, app_state_t *state, uint32_t now) {
    if (telemetry == NULL || state == NULL) {
        return TELEMETRY_STATUS_INVALID_ARGUMENT;
    }
    if (!telemetry->initialized) {
        return TELEMETRY_STATUS_NOT_INITIALIZED;
    }
    if ((!telemetry->count && !telemetry->sample_pending) ||
        (telemetry->waiting_retry && (uint32_t)(now - telemetry->retry_since) < telemetry->config.retry)) {
        return TELEMETRY_STATUS_OK;
    }

    uint32_t transmitted;
    if (telemetry->count != 0) {
        const app_telemetry_error_t *error = &telemetry->errors[telemetry->head];
        state->telemetry_status = cmp_telemetry_send_error(telemetry->config.telemetry,
                                                           app_telemetry_sources[error->source],
                                                           error->code,
                                                           &transmitted);
        telemetry->head = (telemetry->head + 1U) % APP_TELEMETRY_ERROR_CAPACITY;
        --telemetry->count;
    } else {
        state->telemetry_status = cmp_telemetry_send_sample(telemetry->config.telemetry, &telemetry->sample, &transmitted);
        telemetry->sample_pending = false;
    }
    telemetry->waiting_retry = state->telemetry_status != TELEMETRY_STATUS_OK;
    if (telemetry->waiting_retry) {
        telemetry->retry_since = telemetry->config.get_ticks();
    }
    return state->telemetry_status;
}
