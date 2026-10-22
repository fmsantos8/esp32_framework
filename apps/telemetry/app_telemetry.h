/**
 * @file app_telemetry.h
 * @author Fabricio Santos (fmsantos8@ucs.br)
 * @brief Bounded sample and error transmission policy.
 */
#ifndef APP_TELEMETRY_H
#define APP_TELEMETRY_H

#include "app_telemetry_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize an empty queue without UART traffic.
 *
 * @param[out] telemetry Required context; no prior zeroing needed.
 * @param[in] config Required dependencies and positive retry <= INT32_MAX.
 *
 * @retval APP_STATUS_OK Context initialized.
 * @retval APP_STATUS_INVALID_ARGUMENT Invalid pointer or interval; context unchanged.
 */
app_status_t app_telemetry_init(app_telemetry_t *telemetry, const app_telemetry_config_t *config);

/**
 * @brief Replace the pending sample with the newest valid reading.
 *
 * @param[in,out] telemetry Required initialized context.
 * @param[in] sample Required sample, copied before return.
 *
 * @retval APP_STATUS_OK Sample queued; any unsent older sample was replaced.
 * @retval APP_STATUS_INVALID_ARGUMENT A required pointer is NULL.
 * @retval APP_STATUS_NOT_INITIALIZED Context is unavailable.
 */
app_status_t app_telemetry_queue_sample(app_telemetry_t *telemetry, const sht31d_sample_t *sample);

/**
 * @brief Append an error to the bounded FIFO.
 *
 * @param[in,out] telemetry Required initialized context.
 * @param[in] source Valid application error source.
 * @param[in] code Component-specific status code.
 *
 * @retval APP_STATUS_OK Error queued.
 * @retval APP_STATUS_INVALID_ARGUMENT Invalid pointer or source.
 * @retval APP_STATUS_NOT_INITIALIZED Context is unavailable.
 * @retval APP_STATUS_QUEUE_FULL New error dropped; dropped_errors incremented with saturation.
 */
app_status_t app_telemetry_queue_error(app_telemetry_t *telemetry, app_error_source_t source, int32_t code);

/**
 * @brief Attempt at most one record, prioritizing queued errors over the sample.
 *
 * @details Every attempted record is consumed, even on partial transmission;
 * no whole-record replay. A failed attempt delays subsequent records. UART
 * calls are synchronous; the queue does not impose a whole-call deadline.
 *
 * @param[in,out] telemetry Required initialized context.
 * @param[in,out] state Required shared application state.
 * @param[in] now Current ticks from the configured source.
 *
 * @retval TELEMETRY_STATUS_OK Record sent, queue empty, or retry not due.
 * @retval TELEMETRY_STATUS_INVALID_ARGUMENT A required pointer is NULL.
 * @retval TELEMETRY_STATUS_NOT_INITIALIZED Context or component is unavailable.
 * @retval TELEMETRY_STATUS_NOT_SUPPORTED UART operation is unavailable.
 * @retval TELEMETRY_STATUS_TIMEOUT UART transfer timed out.
 * @retval TELEMETRY_STATUS_COMMUNICATION_ERROR UART transfer failed.
 * @retval TELEMETRY_STATUS_FORMAT_ERROR Record formatting failed.
 */
telemetry_status_t app_telemetry_process(app_telemetry_t *telemetry, app_state_t *state, uint32_t now);

#ifdef __cplusplus
}
#endif

#endif
