/**
 * @file app_indication.c
 * @author Fabricio Santos (fmsantos8@ucs.br)
 * @brief LED indication policy.
 */
#include "app_indication.h"

#include <limits.h>
#include <stddef.h>

#include "cmp_led.h"

app_status_t app_indication_init(app_indication_t *indication, const app_indication_config_t *config) {
    if (indication == NULL || config == NULL || config->led == NULL || config->get_ticks == NULL ||
        config->blink_period == 0 || config->blink_period > INT32_MAX || config->retry == 0 ||
        config->retry > INT32_MAX) {
        return APP_STATUS_INVALID_ARGUMENT;
    }
    *indication = (app_indication_t){.config = *config, .initialized = true};
    return APP_STATUS_OK;
}

led_status_t app_indication_process(app_indication_t *indication, app_state_t *state, uint32_t now) {
    if (indication == NULL || state == NULL) {
        return LED_STATUS_INVALID_ARGUMENT;
    }
    if (!indication->initialized) {
        return LED_STATUS_NOT_INITIALIZED;
    }

    app_indication_mode_t mode = state->fault ? APP_INDICATION_FAULT : (state->started ? APP_INDICATION_HEALTHY : APP_INDICATION_STARTUP);
    if (mode != indication->mode) {
        indication->mode = mode;
        indication->phase_since = now;
        indication->phase_on = true;
    }
    if (mode == APP_INDICATION_FAULT) {
        // Advance the phase timestamp so long faults also tolerate clock wraps.
        uint32_t transitions = (uint32_t)(now - indication->phase_since) / indication->config.blink_period;
        indication->phase_since += transitions * indication->config.blink_period;
        if (transitions % 2U != 0) {
            indication->phase_on = !indication->phase_on;
        }
    }
    bool level = mode == APP_INDICATION_HEALTHY || (mode == APP_INDICATION_FAULT && indication->phase_on);
    if (indication->waiting_retry && (uint32_t)(now - indication->retry_since) < indication->config.retry) {
        return LED_STATUS_OK;
    }
    if (indication->applied && indication->level == level) {
        return LED_STATUS_OK;
    }

    state->indication_status = cmp_led_set(indication->config.led, level);
    indication->applied = state->indication_status == LED_STATUS_OK;
    indication->waiting_retry = !indication->applied;
    if (indication->applied) {
        indication->level = level;
    } else {
        indication->retry_since = indication->config.get_ticks();
    }
    return state->indication_status;
}
