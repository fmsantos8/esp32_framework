/**
 * @file app.h
 * @author Fabricio Santos (fmsantos8@ucs.br)
 * @brief Portable cooperative environmental monitoring application.
 *
 * Call sequentially in normal context. Composition owns every component,
 * configuration, and the storage defined in app_config.h. Shared I2C access
 * is serialized here. No HAL, BSP, SDK, RTOS, or dynamic allocation is required.
 */
#ifndef APP_H
#define APP_H

#include "app_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Construct all application modules without device traffic.
 *
 * @details The button, LED, telemetry, and clock must already be available.
 * Sensor and panel initialization is deferred to processing so their failures
 * can be reported and retried. Their configurations must remain valid.
 * Context and config must not overlap. Do not reinitialize a running application.
 *
 * @param[out] app Required caller-owned context; no prior zeroing needed.
 * @param[in] config Required persistent dependencies and intervals.
 *
 * @retval APP_STATUS_OK Coordinator constructed; hardware startup remains pending.
 * @retval APP_STATUS_INVALID_ARGUMENT Invalid pointer or interval; context unchanged.
 */
app_status_t app_init(app_t *app, const app_config_t *config);

/**
 * @brief Advance acquisition, input, rendering, telemetry, and LED indication.
 *
 * @details Invoke regularly from the platform's main loop. Each call samples
 * the injected clock; scheduling handles its 32-bit wrap. No sleeps or busy
 * waits; underlying transfers remain synchronous. There is no hard deadline.
 * The last valid sample survives faults; UART sends at most one queued record
 * and normal OLED updates at most one page per call.
 *
 * @param[in,out] app Required initialized coordinator.
 *
 * @retval APP_STATUS_OK No component currently reports a fault.
 * @retval APP_STATUS_INVALID_ARGUMENT app is NULL.
 * @retval APP_STATUS_NOT_INITIALIZED Context is unavailable.
 * @retval APP_STATUS_COMPONENT_ERROR A component reports a fault; keep processing for recovery.
 */
app_status_t app_process(app_t *app);

/**
 * @brief Copy the current state without accessing devices.
 *
 * @param[in] app Required initialized coordinator.
 * @param[out] state Required independent output; changes only on success.
 *
 * @retval APP_STATUS_OK Snapshot copied.
 * @retval APP_STATUS_INVALID_ARGUMENT A required pointer is NULL.
 * @retval APP_STATUS_NOT_INITIALIZED Context is unavailable.
 */
app_status_t app_get_state(const app_t *app, app_state_t *state);

#ifdef __cplusplus
}
#endif

#endif
