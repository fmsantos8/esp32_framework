/**
 * @file host.c
 * @author Fabricio Santos (fmsantos8@ucs.br)
 * @brief Host composition for pixel-exact interface previews without hardware.
 */
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include "acquisition/app_acquisition.h"
#include "cmp_ssd1306_config.h"
#include "gui/app_gui.h"
#include "status_led/app_status_led.h"
#include "telemetry/app_telemetry.h"

/** @brief Host-only bus token; no platform port is linked. */
struct hal_i2c {
    bool available;  /** True while the virtual transport accepts writes. */
    uint32_t writes; /** Number of accepted display writes. */
};

static hal_i2c_t preview_bus = {.available = true};
static uint32_t preview_ticks;
static acquisition_t preview_acquisition;
static bool preview_has_result;

// Graphics, fonts, and GUI rendering are real; transport and acquisition are simulated.
hal_status_t hal_i2c_write(hal_i2c_t *i2c, uint8_t address, const uint8_t *buffer, uint32_t size) {
    if (i2c != &preview_bus || !i2c->available || address != 0x3C || buffer == NULL || size == 0) {
        return HAL_STATUS_INVALID_ARGUMENT;
    }
    ++i2c->writes;
    return HAL_STATUS_OK;
}

static uint32_t preview_get_ticks(void) {
    return preview_ticks;
}

bool app_acquisition_get_result(acquisition_t *result) {
    if (result == NULL || !preview_has_result) {
        return false;
    }
    *result = preview_acquisition;
    return true;
}

void app_status_led_set_i2c_error(app_status_led_i2c_source_t source, bool active) {
    (void)source;
    (void)active;
}

bool app_telemetry_report_error(telemetry_source_t source, int32_t code) {
    (void)source;
    (void)code;
    return true;
}

/** @brief Write the page-major framebuffer as one hexadecimal line. */
static void preview_write_frame(const ssd1306_t *display) {
    for (uint32_t index = 0; index < SSD1306_FRAMEBUFFER_SIZE; ++index) {
        printf("%02x", display->framebuffer[index]);
    }
    putchar('\n');
}

/** @brief Render every decimal digit with each font used by the GUI. */
static int preview_fonts(void) {
    ssd1306_t display;
    const ssd1306_config_t config = {.i2c = &preview_bus, .address = 0x3C};
    const ssd1306_font_t fonts[] = {SSD1306_FONT_SMALL, SSD1306_FONT_MEDIUM, SSD1306_FONT_LARGE};
    if (cmp_ssd1306_init(&display, &config) != SSD1306_STATUS_OK) {
        return 1;
    }
    for (uint32_t font = 0; font < sizeof(fonts) / sizeof(fonts[0]); ++font) {
        for (char digit = '0'; digit <= '9'; ++digit) {
            const char text[] = {digit, '\0'};
            uint32_t width;
            uint32_t height;
            if (cmp_ssd1306_measure_text(text, fonts[font], &width, &height) != SSD1306_STATUS_OK ||
                cmp_ssd1306_clear(&display) != SSD1306_STATUS_OK ||
                cmp_ssd1306_draw_text(&display, 0, 0, text, fonts[font]) != SSD1306_STATUS_OK) {
                fputs("Font rendering failed.\n", stderr);
                return 1;
            }
            printf("%" PRIu32 " %c %" PRIu32 " %" PRIu32 " ", font, digit, width, height);
            preview_write_frame(&display);
        }
    }
    return ferror(stdout) ? 1 : 0;
}

static int preview_render(int screen, uint64_t uptime) {
    ssd1306_t display;
    const ssd1306_config_t display_config = {.i2c = &preview_bus, .address = 0x3C};
    preview_ticks = 0;
    if (cmp_ssd1306_init(&display, &display_config) != SSD1306_STATUS_OK || !app_gui_init(preview_get_ticks)) {
        return 1;
    }
    preview_bus.writes = 0;

    // Observe the clock before each wrap, including long-uptime preview scenarios.
    while (uptime > UINT32_MAX) {
        preview_ticks += UINT32_MAX;
        app_gui_run(&display);
        uptime -= UINT32_MAX;
    }
    preview_ticks += (uint32_t)uptime;
    if (screen == 1) {
        app_gui_toggle_screen();
    }
    // Finish any frame begun while advancing time, then transfer the selected screen.
    for (uint32_t step = 0; step < 2U * SSD1306_HEIGHT / 8U; ++step) {
        app_gui_run(&display);
    }
    if (preview_bus.writes == 0 || display.dirty_pages != 0) {
        fputs("Interface frame did not complete.\n", stderr);
        return 1;
    }
    preview_write_frame(&display);
    return 0;
}

int main(int argc, char **argv) {
    if (argc == 2 && strcmp(argv[1], "--fonts") == 0) {
        return preview_fonts();
    }
    if (argc != 1) {
        fputs("Usage: app_gui_preview [--fonts]\n", stderr);
        return 1;
    }
    int screen;
    int has_sample;
    int attempted;
    int32_t temperature;
    uint32_t humidity;
    uint64_t uptime;
    char acquisition[16];
    int fields;
    while ((fields = scanf("%d %" SCNd32 " %" SCNu32 " %d %d %15s %" SCNu64, &screen, &temperature, &humidity, &has_sample, &attempted, acquisition, &uptime)) == 7) {
        preview_acquisition = (acquisition_t){
            .sample = {.temperature = temperature, .humidity = humidity},
            .has_sample = has_sample != 0,
        };
        preview_has_result = attempted != 0;
        if (strcmp(acquisition, "ok") == 0) {
            preview_acquisition.status = SHT31D_STATUS_OK;
        } else if (strcmp(acquisition, "timeout") == 0) {
            preview_acquisition.status = SHT31D_STATUS_TIMEOUT;
        } else if (strcmp(acquisition, "crc") == 0) {
            preview_acquisition.status = SHT31D_STATUS_CRC_ERROR;
        } else if (strcmp(acquisition, "i2c") == 0) {
            preview_acquisition.status = SHT31D_STATUS_COMMUNICATION_ERROR;
        } else if (strcmp(acquisition, "unavailable") == 0) {
            preview_acquisition.status = SHT31D_STATUS_NOT_INITIALIZED;
        } else {
            fputs("Unknown acquisition status.\n", stderr);
            return 1;
        }
        if ((screen != 0 && screen != 1) ||
            (has_sample && (temperature < -45000 || temperature > 130000 || humidity > 100000)) ||
            preview_render(screen, uptime) != 0) {
            return 1;
        }
    }
    return fields == EOF && !ferror(stdin) && !ferror(stdout) ? 0 : 1;
}
