/**
 * @file app_acquisition.c
 * @author Fabricio Santos (fmsantos8@ucs.br)
 * @brief Sensor scheduling and retry policy.
 */
#include "app_acquisition.h"

#include <limits.h>
#include <stddef.h>

app_status_t app_acquisition_init(app_acquisition_t *acquisition, const app_acquisition_config_t *config) {
    if (acquisition == NULL || config == NULL || config->sensor == NULL || config->sensor_config == NULL ||
        config->get_ticks == NULL || config->period < SHT31D_MEASUREMENT_TIME || config->period > INT32_MAX ||
        config->retry < SHT31D_MEASUREMENT_TIME || config->retry > INT32_MAX) {
        return APP_STATUS_INVALID_ARGUMENT;
    }
    *acquisition = (app_acquisition_t){.config = *config, .initialized = true};
    return APP_STATUS_OK;
}

sht31d_status_t app_acquisition_process(app_acquisition_t *acquisition, app_state_t *state, uint32_t now) {
    if (acquisition == NULL || state == NULL) {
        return SHT31D_STATUS_INVALID_ARGUMENT;
    }
    if (!acquisition->initialized) {
        return SHT31D_STATUS_NOT_INITIALIZED;
    }
    if ((uint32_t)(now - acquisition->since) < acquisition->delay) {
        return SHT31D_STATUS_OK;
    }

    sht31d_status_t status;
    if (!acquisition->ready) {
        acquisition->init_attempted = true;
        status = cmp_sht31d_init(acquisition->config.sensor, acquisition->config.sensor_config);
        if (status == SHT31D_STATUS_OK) {
            acquisition->ready = true;
            acquisition->delay = 0;
            return status;
        }
    } else if (!acquisition->converting) {
        status = cmp_sht31d_start_measurement(acquisition->config.sensor);
        if (status == SHT31D_STATUS_OK) {
            // Conversion time starts after the synchronous command has returned.
            acquisition->cycle_started = acquisition->config.get_ticks();
            acquisition->since = acquisition->cycle_started;
            acquisition->delay = SHT31D_MEASUREMENT_TIME;
            acquisition->converting = true;
            return status;
        }
    } else {
        sht31d_sample_t sample;
        status = cmp_sht31d_read_measurement(acquisition->config.sensor, &sample);
        if (status == SHT31D_STATUS_OK) {
            state->sample = sample;
            state->has_sample = true;
            state->acquisition_attempted = true;
            state->acquisition_status = status;
            ++state->acquisition_revision;
            acquisition->converting = false;
            acquisition->since = acquisition->cycle_started;
            acquisition->delay = acquisition->config.period;
            return status;
        }
    }

    state->acquisition_attempted = true;
    state->acquisition_status = status;
    ++state->acquisition_revision;
    acquisition->ready = false;
    acquisition->converting = false;
    acquisition->since = acquisition->config.get_ticks();
    acquisition->delay = acquisition->config.retry;
    return status;
}
