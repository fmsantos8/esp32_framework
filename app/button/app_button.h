/**
 * @file app_button.h
 * @author Fabricio Santos (fmsantos8@ucs.br)
 * @brief Detects valid button presses.
 */
#ifndef APP_BUTTON_H
#define APP_BUTTON_H

#ifdef __cplusplus
extern "C" {
#endif

#include "cmp_button.h"

/**
 * @brief Reset button press detection to the released state.
 */
void app_button_init(void);

/**
 * @brief Read the button state and detect a valid press.
 *
 * @param[in,out] button Initialized button component; use the same instance between calls.
 */
void app_button_run(button_t *button);

#ifdef __cplusplus
}
#endif

#endif
