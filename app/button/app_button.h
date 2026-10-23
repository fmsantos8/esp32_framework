/**
 * @file app_button.h
 * @author Fabricio Santos (fmsantos8@ucs.br)
 * @brief Reads the button state and notifies a valid press
 */
#ifndef APP_BUTTON_H
#define APP_BUTTON_H

#ifdef __cplusplus
extern "C" {
#endif

#include "cmp_button.h"

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Callback function type for button press events.
 *
 */
typedef void (*app_button_press_callback_t)(void *context);

/**
 * @brief Initialize the task with press callback.
 *
 * @param[in] press_callback Required function to call on a valid press
 * @param[in] callback_context Context passed to the callback
 *
 * @return true if the task was initialized successfully, false otherwise
 */
bool app_button_init(app_button_press_callback_t press_callback, void *callback_context);

/**
 * @brief Read the button state and notify a valid press
 *
 * @param[in] button Button component
 */
void app_button_run(button_t *button);

#ifdef __cplusplus
}
#endif

#endif
