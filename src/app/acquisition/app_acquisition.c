/**
 * @file app_acquisition.c
 * @author Fabricio Santos (fmsantos8@ucs.br)
 * @brief Acquires periodic sensor measurements and preserves the last valid sample.
 */
#include "app_acquisition.h"
#include "status_led/app_status_led.h"
#include "telemetry/app_telemetry.h"

#include <stddef.h>

#define APP_ACQUISITION_INTERVAL 2000U /** Minimum interval between attempts, in milliseconds. */

_Static_assert(APP_ACQUISITION_INTERVAL >= SHT31D_MEASUREMENT_TIME, "Retry interval must cover sensor conversion time");

typedef enum {
    APP_ACQUISITION_STARTING = 0, /** Send one measurement command. */
    APP_ACQUISITION_WAITING,      /** Wait for conversion before reading. */
    APP_ACQUISITION_INTERVAL_WAIT /** Wait before another attempt. */
} app_acquisition_phase_t;

static uint32_t (*get_ticks)(void) = NULL;                       /** Time source supplied by the integration. */
static app_acquisition_phase_t phase = APP_ACQUISITION_STARTING; /** Current task phase. */
static uint32_t last_attempt = 0;                                /** Time after the latest measurement start returned. */
static bool result_available = false;                            /** True after a completed attempt. */
static bool initialized = false;                                 /** True after successful task initialization. */
static acquisition_t last_result;                                /** Retained measurement and latest attempt status. */

static void publish_acquisition(sht31d_status_t status) {
    last_result.status = status;
    if (status == SHT31D_STATUS_COMMUNICATION_ERROR || status == SHT31D_STATUS_TIMEOUT) {
        last_result.i2c_error = true;
    } else if (status == SHT31D_STATUS_OK || status == SHT31D_STATUS_CRC_ERROR) {
        last_result.i2c_error = false;
    }
    result_available = true;
    app_status_led_set_i2c_error(APP_STATUS_LED_I2C_SENSOR, last_result.i2c_error);
    if (status == SHT31D_STATUS_OK) {
        const telemetry_sample_t sample = {
            .temperature = last_result.sample.temperature,
            .humidity = last_result.sample.humidity,
        };
        app_telemetry_report_sample(&sample);
    } else {
        app_telemetry_report_error(APP_TELEMETRY_SOURCE_SENSOR, (int32_t)status);
    }
}

bool app_acquisition_init(uint32_t (*get_ticks_function)(void)) {
    if (get_ticks_function == NULL ||
        (initialized && phase == APP_ACQUISITION_WAITING)) {
        return false;
    }
    if (initialized && phase == APP_ACQUISITION_INTERVAL_WAIT &&
        (uint32_t)(get_ticks() - last_attempt) < SHT31D_MEASUREMENT_TIME) {
        return false;
    }

    get_ticks = get_ticks_function;
    phase = APP_ACQUISITION_STARTING;
    last_attempt = 0;
    result_available = false;
    last_result = (acquisition_t){0};
    initialized = true;
    return true;
}

void app_acquisition_run(sht31d_t *sensor) {
    if (sensor == NULL || !initialized) {
        return;
    }

    if (phase == APP_ACQUISITION_INTERVAL_WAIT) {
        if ((uint32_t)(get_ticks() - last_attempt) < APP_ACQUISITION_INTERVAL) {
            return;
        }
        phase = APP_ACQUISITION_STARTING;
    }

    if (phase == APP_ACQUISITION_STARTING) {
        const sht31d_status_t status = cmp_sht31d_start_measurement(sensor);
        last_attempt = get_ticks();
        if (status == SHT31D_STATUS_OK) {
            phase = APP_ACQUISITION_WAITING;
        } else {
            phase = APP_ACQUISITION_INTERVAL_WAIT;
            publish_acquisition(status);
        }
        return;
    }

    if (phase == APP_ACQUISITION_WAITING) {
        if ((uint32_t)(get_ticks() - last_attempt) < SHT31D_MEASUREMENT_TIME) {
            return;
        }

        sht31d_sample_t sample;
        const sht31d_status_t status = cmp_sht31d_read_measurement(sensor, &sample);
        phase = APP_ACQUISITION_INTERVAL_WAIT;
        if (status == SHT31D_STATUS_OK) {
            last_result.sample = sample;
            last_result.acquired_at = get_ticks();
            last_result.has_sample = true;
        }
        publish_acquisition(status);
    }
}

bool app_acquisition_get_result(acquisition_t *result) {
    if (result == NULL || !result_available) {
        return false;
    }
    *result = last_result;
    return true;
}
