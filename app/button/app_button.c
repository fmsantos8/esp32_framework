/**
 * @file app_button.c
 * @author Fabricio Santos (fmsantos8@ucs.br)
 * @brief Detects valid button presses.
 */
#include "app_button.h"
#include "gui/app_gui.h"

#include <stddef.h>

static bool previous_pressed = false; /** Last valid button state. */
static bool initialized = false;      /** True after task initialization. */

void app_button_init(void) {
    previous_pressed = false;
    initialized = true;
}

void app_button_run(button_t *button) {
    if (!initialized || button == NULL) {
        return;
    }

    bool pressed = false;
    // Keep the last valid edge state during a read failure.
    if (cmp_button_read(button, &pressed) != BUTTON_STATUS_OK) {
        return;
    }

    const bool press_event = pressed && !previous_pressed;
    previous_pressed = pressed;
    if (press_event) {
        app_gui_toggle_screen();
    }
}
