/**
 * @file app_button.c
 * @author Fabricio Santos (fmsantos8@ucs.br)
 * @brief Detects valid button presses.
 */
#include "app_button.h"

#include <stddef.h>

void app_button_run(button_t *button) {
    if (button == NULL) {
        return;
    }

    static bool previous_pressed = false;
    bool pressed = false;
    // Keep the last valid edge state during a read failure.
    if (cmp_button_read(button, &pressed) != BUTTON_STATUS_OK) {
        return;
    }

    const bool press_event = pressed && !previous_pressed;
    previous_pressed = pressed;
    if (press_event) {
        // TODO: Alternate the screen
    }
}
