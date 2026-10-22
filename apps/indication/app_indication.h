/**
 * @file app_indication.h
 * @author Fabricio Santos (fmsantos8@ucs.br)
 * @brief Startup, healthy, and fault LED policy.
 */
#ifndef APP_INDICATION_H
#define APP_INDICATION_H

#include "app_indication_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize indication without writing the LED.
 *
 * @param[out] indication Required context; no prior zeroing needed.
 * @param[in] config Required dependencies; positive intervals <= INT32_MAX.
 *
 * @retval APP_STATUS_OK Context initialized.
 * @retval APP_STATUS_INVALID_ARGUMENT Invalid pointer or interval; context unchanged.
 */
app_status_t app_indication_init(app_indication_t *indication, const app_indication_config_t *config);

/**
 * @brief Keep the LED off during startup, on when healthy, or blinking on faults.
 *
 * @details Writes only changed or unknown levels; failed writes use retry delay.
 *
 * @param[in,out] indication Required initialized context.
 * @param[in,out] state Required shared application state.
 * @param[in] now Current ticks from the configured source.
 *
 * @retval LED_STATUS_OK Level applied, unchanged, or retry not due.
 * @retval LED_STATUS_INVALID_ARGUMENT A required pointer is NULL.
 * @retval LED_STATUS_NOT_INITIALIZED Context or component is unavailable.
 * @retval LED_STATUS_NOT_SUPPORTED Output operation is unavailable.
 * @retval LED_STATUS_GPIO_ERROR Output write failed.
 */
led_status_t app_indication_process(app_indication_t *indication, app_state_t *state, uint32_t now);

#ifdef __cplusplus
}
#endif

#endif
