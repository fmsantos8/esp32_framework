/**
 * @file app_telemetry.h
 * @author Fabricio Santos (fmsantos8@ucs.br)
 * @brief Queues readings and error occurrences for cooperative UART transmission.
 */
#ifndef APP_TELEMETRY_H
#define APP_TELEMETRY_H

#include "cmp_telemetry.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Devices that produce error records. */
typedef enum {
    APP_TELEMETRY_SOURCE_SENSOR = 0, /** SHT31-D component status codes. */
    APP_TELEMETRY_SOURCE_DISPLAY     /** SSD1306 component status codes. */
} telemetry_source_t;

/**
 * @brief Initialize the telemetry task
 *
 * @param[in] get_ticks_function Required
 *
 * @retval true Task initialized successfully.
 * @retval false get_ticks_function is NULL; previous state is preserved.
 */
bool app_telemetry_init(uint32_t (*get_ticks_function)(void));

/**
 * @brief Send a reading sample to the telemetry queue
 *
 * @param[in] sample Sample to send
 *
 * @retval true Reading queued.
 * @retval false sample is NULL, the task is unavailable, or the queue is full.
 */
bool app_telemetry_report_sample(const telemetry_sample_t *sample);

/**
 * @brief Send an error occurrence to the telemetry queue
 *
 * @param[in] source Device that reported the error.
 * @param[in] code Error code from the device's component.
 *
 * @retval true Error queued.
 * @retval false source is invalid, the task is unavailable, or the queue is full.
 */
bool app_telemetry_report_error(telemetry_source_t source, int32_t code);

/**
 * @brief Transmit queued readings and errors to the telemetry component
 *
 * @param[in] telemetry Telemetry component
 */
void app_telemetry_run(const telemetry_t *telemetry);

#ifdef __cplusplus
}
#endif

#endif
