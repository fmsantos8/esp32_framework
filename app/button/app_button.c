/**
 * @file app_button.c
 * @author Fabricio Santos (fmsantos8@ucs.br)
 * @brief Reads the button state and notifies a valid press
 */
#include "app_button.h"

#include <stddef.h>

static bool previous_pressed = false; /** Last valid state of the button; false = released, true = pressed. */
static bool initialized = false;      /** Task state; false = uninitialized, true = initialized. */

static void *context = NULL;                        /** Context passed to the callback. */
static app_button_press_callback_t callback = NULL; /** Callback function to call on a valid press. */

bool app_button_init(app_button_press_callback_t press_callback, void *callback_context) {
    if (press_callback == NULL) {
        return false;
    }

    callback = press_callback;
    context = callback_context;
    previous_pressed = false;
    initialized = true;

    return true;
}

void app_button_run(button_t *button) {
    if (button == NULL || !initialized) {
        return;
    }

    bool pressed = false;
    // Keep the last valid edge state during a read failure.
    if (cmp_button_read(button, &pressed) != BUTTON_STATUS_OK) {
        return;
    }

    const bool press_event = pressed && !previous_pressed;
    previous_pressed = pressed;
    if (press_event && callback != NULL) {
        callback(context);
    }
}
