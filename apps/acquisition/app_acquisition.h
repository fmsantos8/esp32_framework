/**
 * @file app_acquisition.h
 * @author Fabricio Santos (fmsantos8@ucs.br)
 * @brief Periodic single-shot acquisition and recovery policy.
 */
#ifndef APP_ACQUISITION_H
#define APP_ACQUISITION_H

#include "app_acquisition_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize scheduling without accessing the sensor.
 *
 * @param[out] acquisition Required context; no prior zeroing needed.
 * @param[in] config Required persistent dependencies; intervals <= INT32_MAX.
 *
 * @retval APP_STATUS_OK Context initialized.
 * @retval APP_STATUS_INVALID_ARGUMENT Invalid pointer or interval; context unchanged.
 */
app_status_t app_acquisition_init(app_acquisition_t *acquisition, const app_acquisition_config_t *config);

/**
 * @brief Perform at most one sensor operation when due.
 *
 * @details Waits at least 16 ms after start returns, including after a failed
 * start. Failures retain the last valid sample and schedule reinitialization.
 * No sleep or busy wait; component transfers remain synchronous.
 *
 * @param[in,out] acquisition Required initialized context.
 * @param[in,out] state Required shared state, initially constructed by app_init().
 * @param[in] now Current ticks from the configured time source.
 *
 * @retval SHT31D_STATUS_OK Operation succeeded, or no operation was due.
 * @retval SHT31D_STATUS_INVALID_ARGUMENT A required pointer is NULL.
 * @retval SHT31D_STATUS_NOT_INITIALIZED Context is unavailable, or sensor initialization failed.
 * @retval SHT31D_STATUS_INVALID_STATE Sensor measurement state is inconsistent.
 * @retval SHT31D_STATUS_NOT_SUPPORTED Sensor operation is unavailable.
 * @retval SHT31D_STATUS_TIMEOUT Sensor transfer timed out.
 * @retval SHT31D_STATUS_COMMUNICATION_ERROR Sensor transfer failed.
 * @retval SHT31D_STATUS_CRC_ERROR Sample integrity check failed.
 */
sht31d_status_t app_acquisition_process(app_acquisition_t *acquisition, app_state_t *state, uint32_t now);

#ifdef __cplusplus
}
#endif

#endif
