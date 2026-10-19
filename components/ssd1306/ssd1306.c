#include "ssd1306_config.h"
#include "ssd1306_fonts_private.h"

#include <stddef.h>
#include <string.h>

/** @brief The controller maps each RAM page to eight vertical pixels. */
enum {
    PAGE_HEIGHT = 8,
    PAGE_COUNT = SSD1306_HEIGHT / PAGE_HEIGHT,
    COMMAND_CONTROL = 0x00,
    DATA_CONTROL = 0x40
};

static ssd1306_status_t translate_status(hal_status_t status) {
    switch (status) {
        case HAL_STATUS_OK:
            return SSD1306_STATUS_OK;
        case HAL_STATUS_INVALID_ARGUMENT:
            return SSD1306_STATUS_INVALID_ARGUMENT;
        case HAL_STATUS_NOT_INITIALIZED:
            return SSD1306_STATUS_NOT_INITIALIZED;
        case HAL_STATUS_NOT_SUPPORTED:
            return SSD1306_STATUS_NOT_SUPPORTED;
        case HAL_STATUS_TIMEOUT:
            return SSD1306_STATUS_TIMEOUT;
        default:
            return SSD1306_STATUS_COMMUNICATION_ERROR;
    }
}

static ssd1306_status_t require_initialized(const ssd1306_t *display) {
    if (display == NULL) {
        return SSD1306_STATUS_INVALID_ARGUMENT;
    }
    return display->initialized ? SSD1306_STATUS_OK : SSD1306_STATUS_NOT_INITIALIZED;
}

static bool rectangle_fits(uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
    return x < SSD1306_WIDTH && y < SSD1306_HEIGHT &&
           width <= SSD1306_WIDTH - x && height <= SSD1306_HEIGHT - y;
}

static void put_pixel(ssd1306_t *display, uint32_t x, uint32_t y, bool on) {
    uint32_t page = y / PAGE_HEIGHT;
    uint32_t index = page * SSD1306_WIDTH + x;
    uint8_t mask = (uint8_t)(1U << (y % PAGE_HEIGHT));
    uint8_t previous = display->framebuffer[index];
    uint8_t value = on ? (uint8_t)(previous | mask) : (uint8_t)(previous & (uint8_t)~mask);
    if (value != previous) {
        display->framebuffer[index] = value;
        display->dirty_pages |= (uint8_t)(1U << page);
    }
}

static ssd1306_status_t write_page(ssd1306_t *display, uint32_t page) {
    const uint8_t commands[] = {COMMAND_CONTROL, (uint8_t)(0xB0U | page), 0x00, 0x10};
    hal_status_t status = hal_i2c_write(display->i2c, display->address, commands, (uint32_t)sizeof(commands));
    if (status != HAL_STATUS_OK) {
        return translate_status(status);
    }

    uint8_t packet[SSD1306_WIDTH + 1];
    packet[0] = DATA_CONTROL;
    memcpy(packet + 1, display->framebuffer + page * SSD1306_WIDTH, SSD1306_WIDTH);
    return translate_status(hal_i2c_write(display->i2c, display->address, packet, (uint32_t)sizeof(packet)));
}

static ssd1306_status_t next_codepoint(const uint8_t **cursor, uint32_t *codepoint) {
    if (**cursor >= 0x20 && **cursor <= 0x7E) {
        *codepoint = **cursor;
        ++*cursor;
        return SSD1306_STATUS_OK;
    }
    if ((*cursor)[0] == 0xC2 && (*cursor)[1] == 0xB0) {
        *codepoint = 0xB0;
        *cursor += 2;
        return SSD1306_STATUS_OK;
    }
    return SSD1306_STATUS_NOT_SUPPORTED;
}

static const ssd1306_glyph_t *find_glyph(const ssd1306_font_data_t *font, uint32_t codepoint) {
    for (uint32_t i = 0; i < font->count; ++i) {
        if (font->glyphs[i].codepoint == codepoint) {
            return &font->glyphs[i];
        }
    }
    return NULL;
}

static ssd1306_status_t measure_text(const char *text, const ssd1306_font_data_t *font, uint32_t *out_width) {
    const uint8_t *cursor = (const uint8_t *)text;
    uint32_t width = 0;
    while (*cursor != 0) {
        uint32_t codepoint;
        ssd1306_status_t status = next_codepoint(&cursor, &codepoint);
        if (status != SSD1306_STATUS_OK) {
            return status;
        }
        const ssd1306_glyph_t *glyph = find_glyph(font, codepoint);
        if (glyph == NULL) {
            return SSD1306_STATUS_NOT_SUPPORTED;
        }
        uint32_t advance = glyph->width + (width == 0 ? 0U : 1U);
        if (width > UINT32_MAX - advance) {
            return SSD1306_STATUS_INVALID_ARGUMENT;
        }
        width += advance;
    }
    *out_width = width;
    return SSD1306_STATUS_OK;
}

ssd1306_status_t ssd1306_init(ssd1306_t *display, const ssd1306_config_t *config) {
    if (display == NULL || config == NULL || config->i2c == NULL ||
        (config->address != 0x3C && config->address != 0x3D)) {
        return SSD1306_STATUS_INVALID_ARGUMENT;
    }
    *display = (ssd1306_t){.i2c = config->i2c, .address = config->address};

    // Internal charge pump, page addressing, remapped segments and COM scan.
    // Keep the panel off while replacing its unspecified power-on RAM contents.
    const uint8_t commands[] = {
        COMMAND_CONTROL,
        0xAE,
        0xD5,
        0x80,
        0xA8,
        0x3F,
        0xD3,
        0x00,
        0x40,
        0x8D,
        0x14,
        0x20,
        0x02,
        0xA1,
        0xC8,
        0xDA,
        0x12,
        0x81,
        0xCF,
        0xD9,
        0xF1,
        0xDB,
        0x30,
        0x2E,
        0x23,
        0x00,
        0xD6,
        0x00,
        0xA4,
        0xA6,
    };
    ssd1306_status_t status = translate_status(hal_i2c_write(display->i2c, display->address, commands, (uint32_t)sizeof(commands)));
    if (status != SSD1306_STATUS_OK) {
        return status;
    }
    for (uint32_t page = 0; page < PAGE_COUNT; ++page) {
        status = write_page(display, page);
        if (status != SSD1306_STATUS_OK) {
            return status;
        }
    }

    const uint8_t display_on[] = {COMMAND_CONTROL, 0xAF};
    status = translate_status(hal_i2c_write(display->i2c, display->address, display_on, (uint32_t)sizeof(display_on)));
    if (status == SSD1306_STATUS_OK) {
        display->initialized = true;
    }
    return status;
}

ssd1306_status_t ssd1306_clear(ssd1306_t *display) {
    ssd1306_status_t status = require_initialized(display);
    if (status != SSD1306_STATUS_OK) {
        return status;
    }
    for (uint32_t i = 0; i < SSD1306_FRAMEBUFFER_SIZE; ++i) {
        if (display->framebuffer[i] != 0) {
            display->framebuffer[i] = 0;
            display->dirty_pages |= (uint8_t)(1U << (i / SSD1306_WIDTH));
        }
    }
    return SSD1306_STATUS_OK;
}

ssd1306_status_t ssd1306_fill_rect(ssd1306_t *display, uint32_t x, uint32_t y, uint32_t width, uint32_t height, bool on) {
    ssd1306_status_t status = require_initialized(display);
    if (status != SSD1306_STATUS_OK) {
        return status;
    }
    if (width == 0 || height == 0 || !rectangle_fits(x, y, width, height)) {
        return SSD1306_STATUS_INVALID_ARGUMENT;
    }
    for (uint32_t row = 0; row < height; ++row) {
        for (uint32_t column = 0; column < width; ++column) {
            put_pixel(display, x + column, y + row, on);
        }
    }
    return SSD1306_STATUS_OK;
}

ssd1306_status_t ssd1306_draw_line(ssd1306_t *display, uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1, bool on) {
    ssd1306_status_t status = require_initialized(display);
    if (status != SSD1306_STATUS_OK) {
        return status;
    }
    if (x0 >= SSD1306_WIDTH || x1 >= SSD1306_WIDTH || y0 >= SSD1306_HEIGHT || y1 >= SSD1306_HEIGHT) {
        return SSD1306_STATUS_INVALID_ARGUMENT;
    }

    int32_t x = (int32_t)x0;
    int32_t y = (int32_t)y0;
    int32_t end_x = (int32_t)x1;
    int32_t end_y = (int32_t)y1;
    int32_t dx = x < end_x ? end_x - x : x - end_x;
    int32_t dy = y < end_y ? y - end_y : end_y - y;
    int32_t step_x = x < end_x ? 1 : -1;
    int32_t step_y = y < end_y ? 1 : -1;
    int32_t error = dx + dy;
    while (true) {
        put_pixel(display, (uint32_t)x, (uint32_t)y, on);
        if (x == end_x && y == end_y) {
            break;
        }
        int32_t doubled = 2 * error;
        if (doubled >= dy) {
            error += dy;
            x += step_x;
        }
        if (doubled <= dx) {
            error += dx;
            y += step_y;
        }
    }
    return SSD1306_STATUS_OK;
}

ssd1306_status_t ssd1306_draw_text(ssd1306_t *display, uint32_t x, uint32_t y, const char *text, ssd1306_font_t font) {
    ssd1306_status_t status = require_initialized(display);
    if (status != SSD1306_STATUS_OK) {
        return status;
    }
    if (text == NULL) {
        return SSD1306_STATUS_INVALID_ARGUMENT;
    }
    const ssd1306_font_data_t *data = ssd1306_font_get(font);
    if (data == NULL) {
        return SSD1306_STATUS_NOT_SUPPORTED;
    }
    uint32_t width;
    status = measure_text(text, data, &width);
    if (status != SSD1306_STATUS_OK) {
        return status;
    }
    if (!rectangle_fits(x, y, width, data->height)) {
        return SSD1306_STATUS_INVALID_ARGUMENT;
    }

    // Validation above ensures the entire text can be drawn without partial edits.
    const uint8_t *cursor = (const uint8_t *)text;
    while (*cursor != 0) {
        uint32_t codepoint;
        next_codepoint(&cursor, &codepoint);
        const ssd1306_glyph_t *glyph = find_glyph(data, codepoint);
        uint32_t stride = (glyph->width + 7U) / 8U;
        for (uint32_t row = 0; row < data->height; ++row) {
            for (uint32_t column = 0; column < glyph->width; ++column) {
                uint8_t pixels = data->bitmap[glyph->offset + row * stride + column / 8U];
                if ((pixels & (0x80U >> (column % 8U))) != 0) {
                    put_pixel(display, x + column, y + row, true);
                }
            }
        }
        x += glyph->width + 1U;
    }
    return SSD1306_STATUS_OK;
}

ssd1306_status_t ssd1306_measure_text(const char *text, ssd1306_font_t font, uint32_t *out_width, uint32_t *out_height) {
    if (text == NULL || out_width == NULL || out_height == NULL || out_width == out_height) {
        return SSD1306_STATUS_INVALID_ARGUMENT;
    }
    const ssd1306_font_data_t *data = ssd1306_font_get(font);
    if (data == NULL) {
        return SSD1306_STATUS_NOT_SUPPORTED;
    }
    uint32_t width;
    ssd1306_status_t status = measure_text(text, data, &width);
    if (status == SSD1306_STATUS_OK) {
        *out_width = width;
        *out_height = data->height;
    }
    return status;
}

ssd1306_status_t ssd1306_draw_bitmap(ssd1306_t *display, uint32_t x, uint32_t y, const ssd1306_bitmap_t *bitmap) {
    ssd1306_status_t status = require_initialized(display);
    if (status != SSD1306_STATUS_OK) {
        return status;
    }
    if (bitmap == NULL || bitmap->data == NULL || bitmap->width == 0 || bitmap->height == 0 ||
        !rectangle_fits(x, y, bitmap->width, bitmap->height)) {
        return SSD1306_STATUS_INVALID_ARGUMENT;
    }
    uint32_t stride = (bitmap->width + 7U) / 8U;
    if (bitmap->size < stride * bitmap->height) {
        return SSD1306_STATUS_INVALID_ARGUMENT;
    }
    for (uint32_t row = 0; row < bitmap->height; ++row) {
        for (uint32_t column = 0; column < bitmap->width; ++column) {
            uint8_t pixels = bitmap->data[row * stride + column / 8U];
            put_pixel(display, x + column, y + row, (pixels & (0x80U >> (column % 8U))) != 0);
        }
    }
    return SSD1306_STATUS_OK;
}

ssd1306_status_t ssd1306_update(ssd1306_t *display, bool *out_pending) {
    if (out_pending == NULL) {
        return SSD1306_STATUS_INVALID_ARGUMENT;
    }
    ssd1306_status_t status = require_initialized(display);
    if (status != SSD1306_STATUS_OK) {
        return status;
    }
    for (uint32_t page = 0; page < PAGE_COUNT; ++page) {
        uint8_t mask = (uint8_t)(1U << page);
        if ((display->dirty_pages & mask) != 0) {
            status = write_page(display, page);
            if (status != SSD1306_STATUS_OK) {
                return status;
            }
            display->dirty_pages &= (uint8_t)~mask;
            break;
        }
    }
    *out_pending = display->dirty_pages != 0;
    return SSD1306_STATUS_OK;
}
