/**
 * @file app_interface.h
 * @author Fabricio Santos (fmsantos8@ucs.br)
 * @brief Two-screen environmental monitor interface.
 */
#ifndef APP_INTERFACE_H
#define APP_INTERFACE_H

#include "app_interface_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize presentation state without accessing the display.
 *
 * @param[out] interface Required context; no prior zeroing needed.
 * @param[in] config Required dependencies and positive retry <= INT32_MAX.
 *
 * @retval APP_STATUS_OK Context initialized.
 * @retval APP_STATUS_INVALID_ARGUMENT Invalid pointer or interval; context unchanged.
 */
app_status_t app_interface_init(app_interface_t *interface, const app_interface_config_t *config);

/**
 * @brief Initialize the panel, or draw and transfer at most one dirty page.
 *
 * @details Controller initialization is synchronous and may transfer several
 * pages. Normal updates finish the current frame before drawing a newer one.
 * Failures schedule controller reinitialization and redraw, retaining readings.
 *
 * @param[in,out] interface Required initialized context.
 * @param[in,out] state Required shared application state.
 * @param[in] now Current ticks from the configured source.
 *
 * @retval SSD1306_STATUS_OK Operation succeeded, or no work was due.
 * @retval SSD1306_STATUS_INVALID_ARGUMENT Invalid pointer or rendered value.
 * @retval SSD1306_STATUS_NOT_INITIALIZED Context or component is unavailable.
 * @retval SSD1306_STATUS_NOT_SUPPORTED Graphics or bus operation is unavailable.
 * @retval SSD1306_STATUS_TIMEOUT Display transfer timed out.
 * @retval SSD1306_STATUS_COMMUNICATION_ERROR Display transfer failed.
 */
ssd1306_status_t app_interface_process(app_interface_t *interface, app_state_t *state, uint32_t now);

#ifdef __cplusplus
}
#endif

#endif
