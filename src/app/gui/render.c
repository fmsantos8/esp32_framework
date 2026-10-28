/**
 * @file render.c
 * @author Fabricio Santos (fmsantos8@ucs.br)
 * @brief Draws reading cards and acquisition diagnostics in the framebuffer.
 */
#include "render.h"
#include "bitmaps.h"

#include <inttypes.h>
#include <stdio.h>

#define DRAW_TRY(operation)                    \
    do {                                       \
        ssd1306_status_t result = (operation); \
        if (result != SSD1306_STATUS_OK) {     \
            return result;                     \
        }                                      \
    } while (0)

static ssd1306_status_t draw_right_text(ssd1306_t *display, uint32_t left, uint32_t right, uint32_t y, const char *text, ssd1306_font_t font) {
    uint32_t width = 0;
    uint32_t height = 0;
    DRAW_TRY(cmp_ssd1306_measure_text(text, font, &width, &height));
    if (width > right - left) {
        return SSD1306_STATUS_INVALID_ARGUMENT;
    }
    return cmp_ssd1306_draw_text(display, right - width, y, text, font);
}

static ssd1306_status_t draw_header(ssd1306_t *display, const gui_frame_t *frame, const char *title) {
    DRAW_TRY(cmp_ssd1306_draw_text(display, 6, 3, title, SSD1306_FONT_SMALL));
    if (frame->has_result && frame->acquisition.status != SHT31D_STATUS_OK) {
        DRAW_TRY(cmp_ssd1306_draw_text(display, 92, 3, "!", SSD1306_FONT_SMALL));
    }
    for (uint32_t page = 0; page < 2; ++page) {
        uint32_t x = 108U + 11U * page;
        if ((uint32_t)frame->screen == page) {
            DRAW_TRY(cmp_ssd1306_fill_rect(display, x, 4, 7, 3, true));
        } else {
            DRAW_TRY(cmp_ssd1306_fill_rect(display, x + 2, 5, 3, 1, true));
        }
    }
    return SSD1306_STATUS_OK;
}

static ssd1306_status_t draw_readings(ssd1306_t *display, const gui_frame_t *frame) {
    char temperature[12] = "--,-";
    char humidity[12] = "--,-";
    if (frame->acquisition.has_sample) {
        if (frame->acquisition.sample.temperature < -45000 || frame->acquisition.sample.temperature > 130000 ||
            frame->acquisition.sample.humidity > 100000) {
            return SSD1306_STATUS_INVALID_ARGUMENT;
        }
        // Round to tenths for presentation; keep the sample at its original resolution.
        int64_t value = frame->acquisition.sample.temperature;
        uint32_t magnitude = (uint32_t)(((value < 0 ? -value : value) + 50) / 100);
        uint32_t rh = (frame->acquisition.sample.humidity + 50U) / 100U;
        snprintf(temperature, sizeof(temperature), "%s%" PRIu32 ",%" PRIu32, value < 0 && magnitude != 0 ? "-" : "", (uint32_t)(magnitude / 10U), (uint32_t)(magnitude % 10U));
        snprintf(humidity, sizeof(humidity), "%" PRIu32 ",%" PRIu32, (uint32_t)(rh / 10U), (uint32_t)(rh % 10U));
    }
    DRAW_TRY(draw_header(display, frame, "TEMPERATURA"));
    DRAW_TRY(cmp_ssd1306_draw_bitmap(display, 6, 22, get_thermometer_bitmap()));
    DRAW_TRY(draw_right_text(display, 20, 103, 14, temperature, SSD1306_FONT_LARGE));
    DRAW_TRY(cmp_ssd1306_draw_text(display, 108, 24, "°C", SSD1306_FONT_MEDIUM));
    DRAW_TRY(cmp_ssd1306_draw_line(display, 6, 46, 121, 46, true));
    DRAW_TRY(cmp_ssd1306_draw_bitmap(display, 6, 50, get_droplet_bitmap()));
    DRAW_TRY(cmp_ssd1306_draw_text(display, 21, 53, "UMIDADE", SSD1306_FONT_SMALL));
    DRAW_TRY(draw_right_text(display, 65, 111, 48, humidity, SSD1306_FONT_MEDIUM));
    return cmp_ssd1306_draw_text(display, 116, 48, "%", SSD1306_FONT_MEDIUM);
}

static const char *acquisition_label(const gui_frame_t *frame) {
    if (!frame->has_result) {
        return "AGUARDANDO";
    }
    switch (frame->acquisition.status) {
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

static ssd1306_status_t draw_diagnostics(ssd1306_t *display, const gui_frame_t *frame) {
    uint64_t seconds = frame->uptime_seconds;
    uint64_t days = seconds / 86400U;
    char uptime_text[24];
    if (days == 0) {
        snprintf(uptime_text, sizeof(uptime_text), "%02" PRIu32 ":%02" PRIu32 ":%02" PRIu32, (uint32_t)(seconds / 3600U), (uint32_t)(seconds / 60U % 60U), (uint32_t)(seconds % 60U));
    } else {
        uint32_t shown_days = days > 9999999U ? 9999999U : (uint32_t)days;
        snprintf(uptime_text, sizeof(uptime_text), "%" PRIu32 "%s %02" PRIu32 ":%02" PRIu32 ":%02" PRIu32, (uint32_t)shown_days, days > 9999999U ? "+" : "d", (uint32_t)(seconds / 3600U % 24U), (uint32_t)(seconds / 60U % 60U), (uint32_t)(seconds % 60U));
    }
    DRAW_TRY(draw_header(display, frame, "DIAGNOSTICO"));
    DRAW_TRY(cmp_ssd1306_draw_text(display, 6, 18, "ULTIMA AQUISICAO", SSD1306_FONT_SMALL));
    DRAW_TRY(cmp_ssd1306_draw_text(display, 6, 28, acquisition_label(frame), SSD1306_FONT_SMALL));
    DRAW_TRY(cmp_ssd1306_draw_line(display, 6, 40, 121, 40, true));
    DRAW_TRY(cmp_ssd1306_draw_text(display, 6, 45, "TEMPO ATIVO", SSD1306_FONT_SMALL));
    return cmp_ssd1306_draw_text(display, 6, 55, uptime_text, SSD1306_FONT_SMALL);
}

ssd1306_status_t render_frame(ssd1306_t *display, const gui_frame_t *frame) {
    DRAW_TRY(cmp_ssd1306_clear(display));
    return frame->screen == SCREEN_READINGS ? draw_readings(display, frame)
                                            : draw_diagnostics(display, frame);
}
