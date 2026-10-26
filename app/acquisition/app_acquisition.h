/**
 * @file app_acquisition.h
 * @author Fabricio Santos (fmsantos8@ucs.br)
 * @brief Acquires periodic sensor measurements and preserves the last valid sample.
 */
#ifndef APP_ACQUISITION_H
#define APP_ACQUISITION_H

#include "cmp_sht31d.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Latest acquisition result and retained valid measurement.
 *
 */
typedef struct {
    sht31d_status_t status; /** Result of the latest completed attempt. */
    sht31d_sample_t sample; /** Last valid measurement; meaningful when has_sample is true. */
    uint32_t acquired_at;   /** Last valid read completion time in milliseconds, modulo 2^32. */
    bool has_sample;        /** True after the first successful measurement. */
    bool i2c_error;         /** Communication error or timeout, retained until a complete read. */
} acquisition_t;

/**
 * @brief Initialize the single acquisition task without accessing the sensor.
 *
 * @param[in] get_ticks_function Required function to read the current time (ms)
 *
 * @retval true Task initialized successfully.
 * @retval false get_ticks_function is NULL, a measurement is pending, or its conversion wait is unfinished; previous state is preserved.
 */
bool app_acquisition_init(uint32_t (*get_ticks_function)(void));

/**
 * @brief Start or read a measurement when the current phase is due.
 *
 * @param[in,out] sensor Initialized sensor component; NULL and calls before task initialization are ignored.
 */
void app_acquisition_run(sht31d_t *sensor);

/**
 * @brief Copy the latest completed result without accessing the sensor.
 *
 * @param[out] result Last successful measurement
 *
 * @retval true Result copied successfully.
 * @retval false result is NULL or no attempt has completed since initialization.
 */
bool app_acquisition_get_result(acquisition_t *result);

#ifdef __cplusplus
}
#endif

#endif
