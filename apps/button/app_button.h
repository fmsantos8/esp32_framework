/**
 * @file app_button.h
 * @author Fabricio Santos (fmsantos8@ucs.br)
 * @brief Screen selection using the button component's debounce.
 */
#ifndef APP_BUTTON_H
#define APP_BUTTON_H

#include "app_button_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize press tracking without accessing the GPIO.
 *
 * @param[out] button Required context; no prior zeroing needed.
 * @param[in] component Required initialized component, borrowed for the context lifetime.
 *
 * @retval APP_STATUS_OK Context initialized.
 * @retval APP_STATUS_INVALID_ARGUMENT A required pointer is NULL; context unchanged.
 */
app_status_t app_button_init(app_button_t *button, button_t *component);

/**
 * @brief Read the debounced state and toggle the screen once per press.
 *
 * @details A GPIO error requires a successful release before another press.
 *
 * @param[in,out] button Required initialized context.
 * @param[in,out] state Required shared application state.
 *
 * @retval BUTTON_STATUS_OK State processed.
 * @retval BUTTON_STATUS_INVALID_ARGUMENT A required pointer is NULL.
 * @retval BUTTON_STATUS_NOT_INITIALIZED Context or component is unavailable.
 * @retval BUTTON_STATUS_NOT_SUPPORTED Input operation is unavailable.
 * @retval BUTTON_STATUS_GPIO_ERROR Input read failed.
 */
button_status_t app_button_process(app_button_t *button, app_state_t *state);

#ifdef __cplusplus
}
#endif

#endif
