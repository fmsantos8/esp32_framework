/**
 * @file app_interface.c
 * @author Fabricio Santos (fmsantos8@ucs.br)
 * @brief Reading cards and acquisition diagnostics for the OLED.
 */
#include "app_interface.h"

#include <inttypes.h>
#include <limits.h>
#include <stddef.h>
#include <stdio.h>

#include "cmp_ssd1306.h"

#define APP_INTERFACE_TRY(operation)           \
    do {                                       \
        ssd1306_status_t result = (operation); \
        if (result != SSD1306_STATUS_OK) {     \
            return result;                     \
        }                                      \
    } while (0)

/** @brief Nine-pixel thermometer, one two-byte row per scanline. */
static const uint8_t app_interface_thermometer[][2] = {
    {0x1C, 0x00},
    {0x22, 0x00},
    {0x22, 0x00},
    {0x22, 0x00},
    {0x22, 0x00},
    {0x2A, 0x00},
    {0x2A, 0x00},
    {0x2A, 0x00},
    {0x2A, 0x00},
    {0x2A, 0x00},
    {0x3E, 0x00},
    {0x6B, 0x00},
    {0x5D, 0x00},
    {0xBE, 0x80},
    {0xBE, 0x80},
    {0xBE, 0x80},
    {0x5D, 0x00},
    {0x63, 0x00},
    {0x1C, 0x00},
};

/** @brief Nine-pixel humidity droplet, one two-byte row per scanline. */
static const uint8_t app_interface_droplet[][2] = {
    {0x08, 0x00},
    {0x14, 0x00},
    {0x14, 0x00},
    {0x22, 0x00},
    {0x22, 0x00},
    {0x41, 0x00},
    {0x41, 0x00},
    {0x80, 0x80},
    {0xA0, 0x80},
    {0xA0, 0x80},
    {0xA0, 0x80},
    {0x41, 0x00},
    {0x3E, 0x00},
};

static ssd1306_status_t app_interface_right_text(ssd1306_t *display, uint32_t left, uint32_t right, uint32_t y, const char *text, ssd1306_font_t font) {
    uint32_t width;
    uint32_t height;
    APP_INTERFACE_TRY(cmp_ssd1306_measure_text(text, font, &width, &height));
    if (width > right - left) {
        return SSD1306_STATUS_INVALID_ARGUMENT;
    }
    return cmp_ssd1306_draw_text(display, right - width, y, text, font);
}

static ssd1306_status_t app_interface_header(ssd1306_t *display, const app_state_t *state, const char *title) {
    APP_INTERFACE_TRY(cmp_ssd1306_draw_text(display, 6, 3, title, SSD1306_FONT_SMALL));
    if (state->fault) {
        APP_INTERFACE_TRY(cmp_ssd1306_draw_text(display, 92, 3, "!", SSD1306_FONT_SMALL));
    }
    for (uint32_t page = 0; page < 2; ++page) {
        uint32_t x = 108U + 11U * page;
        if ((uint32_t)state->screen == page) {
            APP_INTERFACE_TRY(cmp_ssd1306_fill_rect(display, x, 4, 7, 3, true));
        } else {
            APP_INTERFACE_TRY(cmp_ssd1306_fill_rect(display, x + 2, 5, 3, 1, true));
        }
    }
    return SSD1306_STATUS_OK;
}

static ssd1306_status_t app_interface_readings(ssd1306_t *display, const app_state_t *state) {
    char temperature[12] = "--,-";
    char humidity[12] = "--,-";
    if (state->has_sample) {
        // Round to tenths for presentation; keep the sample at its original resolution.
        int64_t value = state->sample.temperature;
        uint32_t magnitude = (uint32_t)(((value < 0 ? -value : value) + 50) / 100);
        uint32_t rh = (state->sample.humidity + 50U) / 100U;
        if (state->sample.temperature < -45000 || state->sample.temperature > 130000 ||
            state->sample.humidity > 100000) {
            return SSD1306_STATUS_INVALID_ARGUMENT;
        }
        snprintf(temperature, sizeof(temperature), "%s%" PRIu32 ",%" PRIu32, value < 0 && magnitude != 0 ? "-" : "", magnitude / 10U, magnitude % 10U);
        snprintf(humidity, sizeof(humidity), "%" PRIu32 ",%" PRIu32, rh / 10U, rh % 10U);
    }
    const ssd1306_bitmap_t thermometer = {
        .data = (const uint8_t *)&app_interface_thermometer,
        .size = sizeof(app_interface_thermometer),
        .width = 9,
        .height = 19,
    };
    const ssd1306_bitmap_t droplet = {
        .data = (const uint8_t *)&app_interface_droplet,
        .size = sizeof(app_interface_droplet),
        .width = 9,
        .height = 13,
    };
    APP_INTERFACE_TRY(app_interface_header(display, state, "TEMPERATURA"));
    APP_INTERFACE_TRY(cmp_ssd1306_draw_bitmap(display, 6, 22, &thermometer));
    APP_INTERFACE_TRY(app_interface_right_text(display, 20, 103, 14, temperature, SSD1306_FONT_LARGE));
    APP_INTERFACE_TRY(cmp_ssd1306_draw_text(display, 108, 24, "°C", SSD1306_FONT_MEDIUM));
    APP_INTERFACE_TRY(cmp_ssd1306_draw_line(display, 6, 46, 121, 46, true));
    APP_INTERFACE_TRY(cmp_ssd1306_draw_bitmap(display, 6, 50, &droplet));
    APP_INTERFACE_TRY(cmp_ssd1306_draw_text(display, 21, 53, "UMIDADE", SSD1306_FONT_SMALL));
    APP_INTERFACE_TRY(app_interface_right_text(display, 65, 111, 48, humidity, SSD1306_FONT_MEDIUM));
    return cmp_ssd1306_draw_text(display, 116, 48, "%", SSD1306_FONT_MEDIUM);
}

static const char *app_interface_acquisition_label(const app_state_t *state) {
    if (!state->acquisition_attempted) {
        return "AGUARDANDO";
    }
    switch (state->acquisition_status) {
        case SHT31D_STATUS_OK:
            return "OK";
        case SHT31D_STATUS_TIMEOUT:
            return "TIMEOUT";
        case SHT31D_STATUS_CRC_ERROR:
            return "ERRO CRC";
        case SHT31D_STATUS_COMMUNICATION_ERROR:
            return "ERRO I2C";
        default:
            return "INDISPONIVEL";
    }
}

static ssd1306_status_t app_interface_diagnostics(ssd1306_t *display, const app_state_t *state) {
    uint64_t seconds = state->uptime / 1000U;
    uint64_t days = seconds / 86400U;
    char uptime[24];
    if (days == 0) {
        snprintf(uptime, sizeof(uptime), "%02" PRIu32 ":%02" PRIu32 ":%02" PRIu32, (uint32_t)(seconds / 3600U), (uint32_t)(seconds / 60U % 60U), (uint32_t)(seconds % 60U));
    } else {
        uint32_t shown_days = days > 9999999U ? 9999999U : (uint32_t)days;
        snprintf(uptime, sizeof(uptime), "%" PRIu32 "%s %02" PRIu32 ":%02" PRIu32 ":%02" PRIu32, shown_days, days > 9999999U ? "+" : "d", (uint32_t)(seconds / 3600U % 24U), (uint32_t)(seconds / 60U % 60U), (uint32_t)(seconds % 60U));
    }
    APP_INTERFACE_TRY(app_interface_header(display, state, "DIAGNOSTICO"));
    APP_INTERFACE_TRY(cmp_ssd1306_draw_text(display, 6, 18, "ULTIMA AQUISICAO", SSD1306_FONT_SMALL));
    APP_INTERFACE_TRY(cmp_ssd1306_draw_text(display, 6, 28, app_interface_acquisition_label(state), SSD1306_FONT_SMALL));
    APP_INTERFACE_TRY(cmp_ssd1306_draw_line(display, 6, 40, 121, 40, true));
    APP_INTERFACE_TRY(cmp_ssd1306_draw_text(display, 6, 45, "TEMPO ATIVO", SSD1306_FONT_SMALL));
    return cmp_ssd1306_draw_text(display, 6, 55, uptime, SSD1306_FONT_SMALL);
}

static ssd1306_status_t app_interface_render(ssd1306_t *display, const app_state_t *state) {
    APP_INTERFACE_TRY(cmp_ssd1306_clear(display));
    return state->screen == APP_SCREEN_READINGS ? app_interface_readings(display, state)
                                                : app_interface_diagnostics(display, state);
}

app_status_t app_interface_init(app_interface_t *interface, const app_interface_config_t *config) {
    if (interface == NULL || config == NULL || config->display == NULL || config->display_config == NULL ||
        config->get_ticks == NULL || config->retry == 0 || config->retry > INT32_MAX) {
        return APP_STATUS_INVALID_ARGUMENT;
    }
    *interface = (app_interface_t){.config = *config, .initialized = true};
    return APP_STATUS_OK;
}

ssd1306_status_t app_interface_process(app_interface_t *interface, app_state_t *state, uint32_t now) {
    if (interface == NULL || state == NULL) {
        return SSD1306_STATUS_INVALID_ARGUMENT;
    }
    if (!interface->initialized) {
        return SSD1306_STATUS_NOT_INITIALIZED;
    }
    if (interface->waiting_retry && (uint32_t)(now - interface->retry_since) < interface->config.retry) {
        return SSD1306_STATUS_OK;
    }

    ssd1306_t *display = interface->config.display;
    ssd1306_status_t status = SSD1306_STATUS_OK;
    if (!interface->ready) {
        interface->init_attempted = true;
        status = cmp_ssd1306_init(display, interface->config.display_config);
        interface->ready = status == SSD1306_STATUS_OK;
        interface->rendered = false;
        interface->pending = false;
    } else {
        bool changed = !interface->rendered || interface->screen != state->screen || interface->fault != state->fault ||
                       interface->acquisition_revision != state->acquisition_revision ||
                       (state->screen == APP_SCREEN_DIAGNOSTICS && interface->uptime_seconds != state->uptime / 1000U);
        if (!interface->pending && changed) {
            status = app_interface_render(display, state);
            if (status == SSD1306_STATUS_OK) {
                interface->rendered = true;
                interface->screen = state->screen;
                interface->fault = state->fault;
                interface->acquisition_revision = state->acquisition_revision;
                interface->uptime_seconds = state->uptime / 1000U;
            }
        }
        if (status == SSD1306_STATUS_OK) {
            status = cmp_ssd1306_update(display, &interface->pending);
        }
    }
    state->interface_status = status;
    interface->waiting_retry = status != SSD1306_STATUS_OK;
    if (interface->waiting_retry) {
        interface->ready = false;
        interface->retry_since = interface->config.get_ticks();
    }
    return status;
}
