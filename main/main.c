#include <stdarg.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "bsp.h"
#include "esp_attr.h"
#include "system/hal_system.h"

enum {
    OLED_WIDTH = 128,
    OLED_PAGES = 64 / 8,
};
#define OLED_LINE_CHARS   10
#define MEASURE_PERIOD_MS 2000

/** @brief Characters used by the visual test (5x7, bit 0 at top). */
static const uint8_t font[128][5] = {
    ['%'] = {0x63, 0x13, 0x08, 0x64, 0x63},
    ['-'] = {0x08, 0x08, 0x08, 0x08, 0x08},
    ['.'] = {0x00, 0x60, 0x60, 0x00, 0x00},
    ['0'] = {0x3E, 0x51, 0x49, 0x45, 0x3E},
    ['1'] = {0x00, 0x42, 0x7F, 0x40, 0x00},
    ['2'] = {0x42, 0x61, 0x51, 0x49, 0x46},
    ['3'] = {0x21, 0x41, 0x45, 0x4B, 0x31},
    ['4'] = {0x18, 0x14, 0x12, 0x7F, 0x10},
    ['5'] = {0x27, 0x45, 0x45, 0x45, 0x39},
    ['6'] = {0x3C, 0x4A, 0x49, 0x49, 0x30},
    ['7'] = {0x01, 0x71, 0x09, 0x05, 0x03},
    ['8'] = {0x36, 0x49, 0x49, 0x49, 0x36},
    ['9'] = {0x06, 0x49, 0x49, 0x29, 0x1E},
    [':'] = {0x00, 0x36, 0x36, 0x00, 0x00},
    ['C'] = {0x3E, 0x41, 0x41, 0x41, 0x22},
    ['E'] = {0x7F, 0x49, 0x49, 0x49, 0x41},
    ['H'] = {0x7F, 0x08, 0x08, 0x08, 0x7F},
    ['O'] = {0x3E, 0x41, 0x41, 0x41, 0x3E},
    ['R'] = {0x7F, 0x09, 0x19, 0x29, 0x46},
    ['S'] = {0x26, 0x49, 0x49, 0x49, 0x32},
    ['T'] = {0x01, 0x01, 0x7F, 0x01, 0x01},
    ['U'] = {0x3F, 0x40, 0x40, 0x40, 0x3F},
};

/** @brief Persistent storage shared by all HAL resource constructors. */
_Alignas(max_align_t) static unsigned char hal_storage[1024];

/** @brief Resources supplied by the selected board for the visual test. */
static bsp_resources_t board;

/** @brief Serviced one-millisecond timer notifications, independent of wall time. */
static _Atomic uint32_t timer_notifications;

_Static_assert(ATOMIC_INT_LOCK_FREE == 2, "Timer callback requires lock-free atomic counter");

static void IRAM_ATTR timer_tick(void) {
    atomic_fetch_add_explicit(&timer_notifications, 1, memory_order_relaxed);
}

static void __attribute__((format(printf, 1, 2))) telemetry_printf(const char *format, ...) {
    char message[256];
    va_list arguments;
    va_start(arguments, format);
    int length = vsnprintf(message, sizeof(message), format, arguments);
    va_end(arguments);
    if (length <= 0) {
        return;
    }
    uint32_t size = (uint32_t)length;
    if (size >= sizeof(message)) {
        size = (uint32_t)sizeof(message) - 1;
    }
    uint32_t transmitted;
    hal_uart_write(board.telemetry_uart, (const uint8_t *)message, size, &transmitted);
}

static bool report_status(const char *step, hal_status_t status) {
    telemetry_printf("%s: %s (status %d)\n", step, status == HAL_STATUS_OK ? "OK" : "FALHOU", status);
    return status == HAL_STATUS_OK;
}

static uint8_t sht31_crc(const uint8_t *bytes) {
    uint8_t crc = 0xFF;
    for (int i = 0; i < 2; ++i) {
        crc ^= bytes[i];
        for (int bit = 0; bit < 8; ++bit) {
            crc = (crc & 0x80) ? (uint8_t)((crc << 1) ^ 0x31)
                               : (uint8_t)(crc << 1);
        }
    }
    return crc;
}

static bool sht31_measure(int32_t *temperature_tenths, uint32_t *humidity_tenths) {
    // Single-shot measurement, high repeatability, without clock stretching.
    const uint8_t command[] = {0x24, 0x00};
    hal_status_t status = hal_i2c_write(board.devices_i2c, board.sht31_address, command, sizeof(command));
    if (status != HAL_STATUS_OK) {
        report_status("SHT31: comando de medicao", status);
        return false;
    }

    vTaskDelay(pdMS_TO_TICKS(30));
    uint8_t sample[6];
    status = hal_i2c_read(board.devices_i2c, board.sht31_address, sample, sizeof(sample));
    if (status != HAL_STATUS_OK) {
        report_status("SHT31: leitura", status);
        return false;
    }
    if (sht31_crc(sample) != sample[2] ||
        sht31_crc(sample + 3) != sample[5]) {
        telemetry_printf("SHT31: CRC FALHOU\n");
        return false;
    }

    uint16_t raw_temperature = ((uint16_t)sample[0] << 8) | sample[1];
    uint16_t raw_humidity = ((uint16_t)sample[3] << 8) | sample[4];
    *temperature_tenths = -450 + (int32_t)((1750UL * raw_temperature + 32767UL) / 65535UL);
    *humidity_tenths = (1000UL * raw_humidity + 32767UL) / 65535UL;
    return true;
}

static hal_status_t oled_write_page(uint8_t page, const uint8_t data[OLED_WIDTH + 1]) {
    const uint8_t address[] = {0x00, (uint8_t)(0xB0 | page), 0x00, 0x10};
    hal_status_t status = hal_i2c_write(board.devices_i2c, board.ssd1306_address, address, sizeof(address));
    if (status != HAL_STATUS_OK) {
        return status;
    }
    return hal_i2c_write(board.devices_i2c, board.ssd1306_address, data, OLED_WIDTH + 1);
}

static hal_status_t oled_init(void) {
    // SSD1306, 128x64, internal charge pump, page addressing mode.
    const uint8_t init[] = {
        0x00,
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
        0x40,
        0x2E,
        0xA4,
        0xA6,
    };
    hal_status_t status = hal_i2c_write(board.devices_i2c, board.ssd1306_address, init, sizeof(init));
    if (status != HAL_STATUS_OK) {
        return status;
    }

    const uint8_t blank[OLED_WIDTH + 1] = {0x40};
    for (uint8_t page = 0; page < OLED_PAGES; ++page) {
        status = oled_write_page(page, blank);
        if (status != HAL_STATUS_OK) {
            return status;
        }
    }
    const uint8_t display_on[] = {0x00, 0xAF};
    return hal_i2c_write(board.devices_i2c, board.ssd1306_address, display_on, sizeof(display_on));
}

static hal_status_t oled_write_line(uint8_t first_page, const char *text) {
    uint8_t top[OLED_WIDTH + 1] = {0x40};
    uint8_t bottom[OLED_WIDTH + 1] = {0x40};
    for (uint8_t character = 0;
         character < OLED_LINE_CHARS && text[character] != '\0';
         ++character) {
        const uint8_t *glyph = font[(uint8_t)text[character]];
        for (uint8_t column = 0; column < 5; ++column) {
            uint16_t pixels = 0;
            for (uint8_t row = 0; row < 7; ++row) {
                if ((glyph[column] & (1U << row)) != 0) {
                    pixels |= (uint16_t)(3U << (row * 2));
                }
            }
            uint8_t x = character * 12 + column * 2 + 1;
            top[x] = top[x + 1] = (uint8_t)pixels;
            bottom[x] = bottom[x + 1] = (uint8_t)(pixels >> 8);
        }
    }

    hal_status_t status = oled_write_page(first_page, top);
    if (status != HAL_STATUS_OK) {
        return status;
    }
    return oled_write_page(first_page + 1, bottom);
}

static hal_status_t oled_show(const char *top, const char *bottom) {
    hal_status_t status = oled_write_line(1, top);
    if (status != HAL_STATUS_OK) {
        return status;
    }
    return oled_write_line(5, bottom);
}

static void update_measurement(bool display_ready) {
    int32_t temperature;
    uint32_t humidity;
    if (!sht31_measure(&temperature, &humidity)) {
        if (display_ready) {
            hal_status_t status = oled_show("SHT31 ERRO", "");
            if (status != HAL_STATUS_OK) {
                report_status("SSD1306: atualizacao", status);
            }
        }
        return;
    }

    int32_t magnitude = temperature < 0 ? -temperature : temperature;
    telemetry_printf("SHT31: %s%ld.%ld C, %lu.%lu %% UR\n",
                     temperature < 0 ? "-" : "",
                     (long)(magnitude / 10),
                     (long)(magnitude % 10),
                     (unsigned long)(humidity / 10),
                     (unsigned long)(humidity % 10));
    if (display_ready) {
        char temperature_line[32];
        char humidity_line[32];
        snprintf(temperature_line, sizeof(temperature_line), "T:%s%ld.%ld C", temperature < 0 ? "-" : "", (long)(magnitude / 10), (long)(magnitude % 10));
        snprintf(humidity_line, sizeof(humidity_line), "UR:%lu.%lu%%", (unsigned long)(humidity / 10), (unsigned long)(humidity % 10));
        hal_status_t status = oled_show(temperature_line, humidity_line);
        if (status != HAL_STATUS_OK) {
            report_status("SSD1306: atualizacao", status);
        }
    }
}

void app_main(void) {
    const hal_system_config_t config = {
        .memory_buffer = hal_storage,
        .memory_capacity = (uint32_t)sizeof(hal_storage),
    };
    if (hal_system_init(&config) != HAL_STATUS_OK || bsp_init(timer_tick, &board) != HAL_STATUS_OK ||
        hal_system_seal() != HAL_STATUS_OK) {
        return;
    }

    static const uint8_t message[] = "UART0: transmissao pelo port OK\r\n";
    uint32_t transmitted;
    report_status("UART0: transmissao",
                  hal_uart_write(board.telemetry_uart, message, (uint32_t)sizeof(message) - 1, &transmitted));

    if (report_status("Timer: iniciar", hal_timer_start(board.timebase_timer))) {
        uint32_t before = atomic_load_explicit(&timer_notifications, memory_order_relaxed);
        vTaskDelay(pdMS_TO_TICKS(30));
        uint32_t after = atomic_load_explicit(&timer_notifications, memory_order_relaxed);
        telemetry_printf("Timer: %s (%lu notificacoes de periodo nominal 1 ms)\n",
                         after != before ? "OK" : "FALHOU",
                         (unsigned long)(uint32_t)(after - before));
    }

    bool button_ready = true;
    bool led_ready = report_status("GPIO19: acender LED",
                                   hal_gpio_write(board.led, board.led_active_level));
    vTaskDelay(pdMS_TO_TICKS(200));
    led_ready = report_status("GPIO19: apagar LED",
                              hal_gpio_write(board.led, !board.led_active_level)) &&
                led_ready;

    bool display_ready = report_status("SSD1306: inicializacao", oled_init());

    telemetry_printf("Pressione o botao: o LED deve acender; solte para apagar.\n");
    bool last_pressed = false;
    bool has_sample = false;
    TickType_t last_measurement = xTaskGetTickCount() - pdMS_TO_TICKS(MEASURE_PERIOD_MS);
    while (true) {
        TickType_t now = xTaskGetTickCount();
        if ((TickType_t)(now - last_measurement) >=
            pdMS_TO_TICKS(MEASURE_PERIOD_MS)) {
            last_measurement = now;
            update_measurement(display_ready);
        }

        if (button_ready) {
            bool level;
            hal_status_t status = hal_gpio_read(board.button, &level);
            if (status != HAL_STATUS_OK) {
                report_status("GPIO18: leitura", status);
                button_ready = false;
            } else {
                bool pressed = level == board.button_active_level;
                if (!has_sample || pressed != last_pressed) {
                    telemetry_printf("Botao: %s\n", pressed ? "pressionado" : "solto");
                    last_pressed = pressed;
                    has_sample = true;
                }
                if (led_ready) {
                    status = hal_gpio_write(board.led, pressed ? board.led_active_level : !board.led_active_level);
                    if (status != HAL_STATUS_OK) {
                        report_status("GPIO19: escrita", status);
                        led_ready = false;
                    }
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
