/**
 * @file app_button.h
 * @author Fabricio Santos (fmsantos8@ucs.br)
 * @brief Cooperative button task with an injected press receiver.
 */
#ifndef APP_BUTTON_H
#define APP_BUTTON_H

#include "cmp_button.h"

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Notify one valid transition from released to pressed.
 *
 * @details Called synchronously after updating task state. Return promptly
 * without waiting or calling app_button_init() or app_button_run().
 *
 * @param[in,out] context Borrowed receiver context; may be NULL if the receiver allows it.
 */
typedef void (*app_button_press_callback_t)(void *context);

/**
 * @brief Associate the single button task with a component and press receiver.
 *
 * @details Stores borrowed references without reading GPIO or invoking the
 * callback. The component and receiver context must outlive their use.
 * Successful initialization replaces the previous association and resets the
 * last valid state to released. Initialize only while the task and callback
 * are idle. The component remains owned and initialized by the integration.
 *
 * @param[in] button Required initialized button component.
 * @param[in] press_callback Optional receiver of a valid press; NULL disables notification.
 * @param[in,out] callback_context Borrowed callback argument; may be NULL if the receiver allows it.
 *
 * @retval true Association stored and task state initialized.
 * @retval false button is NULL; the previous association and state are preserved.
 */
bool app_button_init(button_t *button, app_button_press_callback_t press_callback, void *callback_context);

/**
 * @brief Read the associated component once and notify a valid press.
 *
 * @details Reuses component debounce without adding a filter or waiting for
 * deadlines. Holding the button does not repeat the callback; a valid release
 * rearms detection. A button held at initialization produces one press when
 * the component confirms it. Reading failures preserve the last valid state
 * and invoke no callback. A release missed during a fault does not create
 * another press on recovery.
 *
 * Calls before initialization, with NULL, or with a different component are
 * ignored without reading GPIO. Call sequentially in normal context, with
 * this task as the component's only consumer. State is updated before invoking
 * the callback. The task and its callback must not be reentered.
 *
 * @param[in,out] button Component associated by app_button_init(); NULL is ignored.
 */
void app_button_run(button_t *button);

#ifdef __cplusplus
}
#endif

#endif
