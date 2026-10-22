/**
 * @file app.c
 * @author Fabricio Santos (fmsantos8@ucs.br)
 * @brief Cooperative product coordination, without platform dependencies.
 */
#include "app.h"
#include "app_config.h"

#include <stddef.h>

#include "acquisition/app_acquisition.h"
#include "button/app_button.h"
#include "indication/app_indication.h"
#include "interface/app_interface.h"
#include "telemetry/app_telemetry.h"

static bool app_has_fault(const app_t *app) {
    const app_state_t *state = &app->state;
    return (state->acquisition_attempted && state->acquisition_status != SHT31D_STATUS_OK) ||
           (app->interface.init_attempted && state->interface_status != SSD1306_STATUS_OK) ||
           state->button_status != BUTTON_STATUS_OK || state->indication_status != LED_STATUS_OK ||
           state->telemetry_status != TELEMETRY_STATUS_OK;
}

app_status_t app_init(app_t *app, const app_config_t *config) {
    if (app == NULL || config == NULL || config->sensor == NULL || config->sensor_config == NULL ||
        config->display == NULL || config->display_config == NULL || config->button == NULL || config->led == NULL ||
        config->telemetry == NULL || config->get_ticks == NULL) {
        return APP_STATUS_INVALID_ARGUMENT;
    }
    uint32_t period = config->acquisition_period ? config->acquisition_period : APP_DEFAULT_ACQUISITION_PERIOD;
    uint32_t retry = config->retry_period ? config->retry_period : APP_DEFAULT_RETRY_PERIOD;
    uint32_t blink = config->blink_period ? config->blink_period : APP_DEFAULT_BLINK_PERIOD;
    app_t context = {.get_ticks = config->get_ticks, .initialized = true};
    const app_acquisition_config_t acquisition_config = {
        .sensor = config->sensor,
        .sensor_config = config->sensor_config,
        .get_ticks = config->get_ticks,
        .period = period,
        .retry = retry,
    };
    const app_interface_config_t interface_config = {
        .display = config->display,
        .display_config = config->display_config,
        .get_ticks = config->get_ticks,
        .retry = retry,
    };
    const app_indication_config_t indication_config = {
        .led = config->led,
        .get_ticks = config->get_ticks,
        .blink_period = blink,
        .retry = retry,
    };
    const app_telemetry_config_t telemetry_config = {
        .telemetry = config->telemetry,
        .get_ticks = config->get_ticks,
        .retry = retry,
    };
    if (app_acquisition_init(&context.acquisition, &acquisition_config) != APP_STATUS_OK ||
        app_button_init(&context.button, config->button) != APP_STATUS_OK ||
        app_interface_init(&context.interface, &interface_config) != APP_STATUS_OK ||
        app_indication_init(&context.indication, &indication_config) != APP_STATUS_OK ||
        app_telemetry_init(&context.telemetry, &telemetry_config) != APP_STATUS_OK) {
        return APP_STATUS_INVALID_ARGUMENT;
    }
    context.state.acquisition_status = SHT31D_STATUS_NOT_INITIALIZED;
    context.state.interface_status = SSD1306_STATUS_NOT_INITIALIZED;
    context.last_ticks = config->get_ticks();
    *app = context;
    return APP_STATUS_OK;
}

app_status_t app_process(app_t *app) {
    if (app == NULL) {
        return APP_STATUS_INVALID_ARGUMENT;
    }
    if (!app->initialized) {
        return APP_STATUS_NOT_INITIALIZED;
    }
    app_state_t *state = &app->state;
    uint32_t now = app->get_ticks();
    state->uptime += (uint32_t)(now - app->last_ticks);
    app->last_ticks = now;

    button_status_t previous_button = state->button_status;
    app_button_process(&app->button, state);
    if (state->button_status != BUTTON_STATUS_OK && state->button_status != previous_button) {
        app_telemetry_queue_error(&app->telemetry, APP_ERROR_BUTTON, state->button_status);
    }

    uint32_t previous_revision = state->acquisition_revision;
    app_acquisition_process(&app->acquisition, state, app->get_ticks());
    if (state->acquisition_revision != previous_revision) {
        if (state->acquisition_status == SHT31D_STATUS_OK) {
            app_telemetry_queue_sample(&app->telemetry, &state->sample);
        } else {
            app_telemetry_queue_error(&app->telemetry, APP_ERROR_ACQUISITION, state->acquisition_status);
        }
    }

    state->fault = app_has_fault(app);
    ssd1306_status_t previous_interface = state->interface_status;
    bool interface_attempted = app->interface.init_attempted;
    app_interface_process(&app->interface, state, app->get_ticks());
    if (app->interface.init_attempted && state->interface_status != SSD1306_STATUS_OK &&
        (!interface_attempted || state->interface_status != previous_interface)) {
        app_telemetry_queue_error(&app->telemetry, APP_ERROR_INTERFACE, state->interface_status);
    }

    app_telemetry_process(&app->telemetry, state, app->get_ticks());
    state->started = state->acquisition_attempted && app->interface.init_attempted;
    state->fault = app_has_fault(app);
    led_status_t previous_indication = state->indication_status;
    app_indication_process(&app->indication, state, app->get_ticks());
    if (state->indication_status != LED_STATUS_OK && state->indication_status != previous_indication) {
        app_telemetry_queue_error(&app->telemetry, APP_ERROR_INDICATION, state->indication_status);
    }
    state->dropped_errors = app->telemetry.dropped_errors;
    state->fault = app_has_fault(app);
    return state->fault ? APP_STATUS_COMPONENT_ERROR : APP_STATUS_OK;
}

app_status_t app_get_state(const app_t *app, app_state_t *state) {
    if (app == NULL || state == NULL) {
        return APP_STATUS_INVALID_ARGUMENT;
    }
    if (!app->initialized) {
        return APP_STATUS_NOT_INITIALIZED;
    }
    *state = app->state;
    return APP_STATUS_OK;
}
