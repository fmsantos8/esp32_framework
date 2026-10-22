/**
 * @file app_button.c
 * @author Fabricio Santos (fmsantos8@ucs.br)
 * @brief Button edge detection and synchronous press notification.
 */
#include "app_button.h"

#include <stddef.h>

/** @brief Single task instance with borrowed component and callback references. */
static struct {
    button_t *button;                           /** Component associated with this task. */
    app_button_press_callback_t press_callback; /** Optional receiver of a valid press. */
    void *callback_context;                     /** Borrowed argument passed to the receiver. */
    bool previous_pressed;                      /** Last successfully read logical state. */
    bool initialized;                           /** True after successful task initialization. */
} context;

bool app_button_init(button_t *button, app_button_press_callback_t press_callback, void *callback_context) {
    if (button == NULL) {
        return false;
    }

    context.button = button;
    context.press_callback = press_callback;
    context.callback_context = callback_context;
    context.previous_pressed = false;
    context.initialized = true;
    return true;
}

void app_button_run(button_t *button) {
    if (button == NULL || !context.initialized || button != context.button) {
        return;
    }

    bool pressed = false;
    // Keep the last valid edge state during a read failure.
    if (cmp_button_read(button, &pressed) != BUTTON_STATUS_OK) {
        return;
    }

    const bool press_event = pressed && !context.previous_pressed;
    context.previous_pressed = pressed;
    if (press_event && context.press_callback != NULL) {
        context.press_callback(context.callback_context);
    }
}
