/**
 * @file app_button.c
 * @author Fabricio Santos (fmsantos8@ucs.br)
 * @brief Screen selection policy.
 */
#include "app_button.h"

#include <stddef.h>

#include "cmp_button.h"

app_status_t app_button_init(app_button_t *button, button_t *component) {
    if (button == NULL || component == NULL) {
        return APP_STATUS_INVALID_ARGUMENT;
    }
    *button = (app_button_t){.button = component, .armed = true, .initialized = true};
    return APP_STATUS_OK;
}

button_status_t app_button_process(app_button_t *button, app_state_t *state) {
    if (button == NULL || state == NULL) {
        return BUTTON_STATUS_INVALID_ARGUMENT;
    }
    if (!button->initialized) {
        return BUTTON_STATUS_NOT_INITIALIZED;
    }

    bool pressed;
    state->button_status = cmp_button_read(button->button, &pressed);
    if (state->button_status != BUTTON_STATUS_OK) {
        button->armed = false;
    } else if (!pressed) {
        button->armed = true;
    } else if (button->armed) {
        state->screen = state->screen == APP_SCREEN_READINGS ? APP_SCREEN_DIAGNOSTICS : APP_SCREEN_READINGS;
        button->armed = false;
    }
    return state->button_status;
}
