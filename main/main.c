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
#include "sht31_config.h"
#include "ssd1306_config.h"
#include "system/hal_system.h"

#define MEASURE_PERIOD_MS 2000

/** @brief Pixels belonging to the readings screen, independent of the driver. */
static const uint8_t thermometer_pixels[] = {
    0x1C,
    0x00,
    0x22,
    0x00,
    0x22,
    0x00,
    0x22,
    0x00,
    0x22,
    0x00,
    0x2A,
    0x00,
    0x2A,
    0x00,
    0x2A,
    0x00,
    0x2A,
    0x00,
    0x2A,
    0x00,
    0x3E,
    0x00,
    0x6B,
    0x00,
    0x5D,
    0x00,
    0xBE,
    0x80,
    0xBE,
    0x80,
    0xBE,
    0x80,
    0x5D,
    0x00,
    0x63,
    0x00,
    0x1C,
    0x00,
};

/** @brief Humidity icon, stored as MSB-first rows with a two-byte stride. */
static const uint8_t drop_pixels[] = {
    0x08,
    0x00,
    0x14,
    0x00,
    0x14,
    0x00,
    0x22,
    0x00,
    0x22,
    0x00,
    0x41,
    0x00,
    0x41,
    0x00,
    0x80,
    0x80,
    0xA0,
    0x80,
    0xA0,
    0x80,
    0xA0,
    0x80,
    0x41,
    0x00,
    0x3E,
    0x00,
};

/** @brief Component storage supplied by the composition, outside the HAL arena. */
static ssd1306_t display;

/** @brief Sensor context supplied by the composition, sharing the display bus. */
static sht31_t sensor;

/** @brief Last successful values, or placeholders before the first acquisition. */
static char temperature_text[16] = "--,-";
static char humidity_text[16] = "--,-";

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

static bool report_sensor_status(const char *step, sht31_status_t status) {
    telemetry_printf("%s: %s (sht31 status %d)\n", step, status == SHT31_STATUS_OK ? "OK" : "FALHOU", status);
    return status == SHT31_STATUS_OK;
}

static bool report_display_status(const char *step, ssd1306_status_t status) {
    telemetry_printf("%s: %s (ssd1306 status %d)\n", step, status == SSD1306_STATUS_OK ? "OK" : "FALHOU", status);
    return status == SSD1306_STATUS_OK;
}

static ssd1306_status_t draw_readings_layout(void) {
    const ssd1306_bitmap_t thermometer = {
        .data = thermometer_pixels,
        .size = (uint32_t)sizeof(thermometer_pixels),
        .width = 9,
        .height = 19,
    };
    const ssd1306_bitmap_t drop = {
        .data = drop_pixels,
        .size = (uint32_t)sizeof(drop_pixels),
        .width = 9,
        .height = 13,
    };
    ssd1306_status_t status;
    if ((status = ssd1306_draw_text(&display, 6, 3, "TEMPERATURA", SSD1306_FONT_SMALL)) != SSD1306_STATUS_OK ||
        (status = ssd1306_draw_bitmap(&display, 6, 22, &thermometer)) != SSD1306_STATUS_OK ||
        (status = ssd1306_draw_line(&display, 6, 46, 121, 46, true)) != SSD1306_STATUS_OK ||
        (status = ssd1306_draw_text(&display, 108, 24, "°C", SSD1306_FONT_MEDIUM)) != SSD1306_STATUS_OK ||
        (status = ssd1306_draw_bitmap(&display, 6, 50, &drop)) != SSD1306_STATUS_OK ||
        (status = ssd1306_draw_text(&display, 21, 53, "UMIDADE", SSD1306_FONT_SMALL)) != SSD1306_STATUS_OK ||
        (status = ssd1306_draw_text(&display, 116, 48, "%", SSD1306_FONT_MEDIUM)) != SSD1306_STATUS_OK) {
        return status;
    }
    return SSD1306_STATUS_OK;
}

static ssd1306_status_t draw_right_aligned(const char *text, ssd1306_font_t font, uint32_t left, uint32_t right, uint32_t y) {
    uint32_t width;
    uint32_t height;
    ssd1306_status_t status = ssd1306_measure_text(text, font, &width, &height);
    if (status != SSD1306_STATUS_OK) {
        return status;
    }
    if (width > right - left) {
        return SSD1306_STATUS_INVALID_ARGUMENT;
    }
    return ssd1306_draw_text(&display, right - width, y, text, font);
}

static ssd1306_status_t draw_readings(bool sample_error) {
    // Redraw only the changing fields, keeping labels and icons in the framebuffer.
    ssd1306_status_t status;
    if ((status = ssd1306_fill_rect(&display, 20, 14, 82, 28, false)) != SSD1306_STATUS_OK ||
        (status = ssd1306_fill_rect(&display, 68, 48, 44, 16, false)) != SSD1306_STATUS_OK ||
        (status = ssd1306_fill_rect(&display, 88, 3, 5, 7, false)) != SSD1306_STATUS_OK ||
        (status = draw_right_aligned(temperature_text, SSD1306_FONT_LARGE, 20, 98, 14)) != SSD1306_STATUS_OK ||
        (status = draw_right_aligned(humidity_text, SSD1306_FONT_MEDIUM, 68, 110, 48)) != SSD1306_STATUS_OK) {
        return status;
    }
    if (sample_error) {
        return ssd1306_draw_text(&display, 88, 3, "!", SSD1306_FONT_SMALL);
    }
    return SSD1306_STATUS_OK;
}

static void report_measurement_failure(const char *step, sht31_status_t status, bool display_ready) {
    report_sensor_status(step, status);
    if (display_ready) {
        ssd1306_status_t drawing_status = draw_readings(true);
        if (drawing_status != SSD1306_STATUS_OK) {
            report_display_status("SSD1306: desenho", drawing_status);
        }
    }
}

static void update_measurement(const sht31_sample_t *sample, bool display_ready) {
    // Round only for presentation; the component preserves millidegree resolution.
    int32_t temperature = sample->temperature_millicelsius;
    temperature = (temperature + (temperature < 0 ? -50 : 50)) / 100;
    uint32_t humidity = (sample->humidity_millipercent + 50U) / 100U;

    int32_t magnitude = temperature < 0 ? -temperature : temperature;
    telemetry_printf("SHT31: %s%ld.%ld C, %lu.%lu %% UR\n",
                     temperature < 0 ? "-" : "",
                     (long)(magnitude / 10),
                     (long)(magnitude % 10),
                     (unsigned long)(humidity / 10),
                     (unsigned long)(humidity % 10));
    if (display_ready) {
        snprintf(temperature_text, sizeof(temperature_text), "%s%ld,%ld", temperature < 0 ? "-" : "", (long)(magnitude / 10), (long)(magnitude % 10));
        snprintf(humidity_text, sizeof(humidity_text), "%lu,%lu", (unsigned long)(humidity / 10), (unsigned long)(humidity % 10));
        ssd1306_status_t status = draw_readings(false);
        if (status != SSD1306_STATUS_OK) {
            report_display_status("SSD1306: desenho", status);
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

    const sht31_config_t sensor_config = {
        .i2c = board.devices_i2c,
        .address = board.sht31_address,
    };
    bool sensor_ready = report_sensor_status("SHT31: inicializacao", sht31_init(&sensor, &sensor_config));

    const ssd1306_config_t display_config = {
        .i2c = board.devices_i2c,
        .address = board.ssd1306_address,
    };
    bool display_ready = report_display_status("SSD1306: inicializacao", ssd1306_init(&display, &display_config));
    if (display_ready) {
        display_ready = report_display_status("SSD1306: layout", draw_readings_layout()) &&
                        report_display_status("SSD1306: valores iniciais", draw_readings(!sensor_ready));
    }
    bool display_waiting_retry = false;
    TickType_t last_display_failure = 0;

    telemetry_printf("Pressione o botao: o LED deve acender; solte para apagar.\n");
    bool last_pressed = false;
    bool has_sample = false;
    TickType_t last_measurement = xTaskGetTickCount() - (sensor_ready ? pdMS_TO_TICKS(MEASURE_PERIOD_MS) : 0);
    TickType_t measurement_started = 0;
    bool measurement_pending = false;
    // Round up and allow one additional tick for the unknown phase of the clock.
    const TickType_t conversion_ticks = pdMS_TO_TICKS(SHT31_MEASUREMENT_TIME_MS) + 2U;
    while (true) {
        TickType_t now = xTaskGetTickCount();
        if (!measurement_pending && (TickType_t)(now - last_measurement) >= pdMS_TO_TICKS(MEASURE_PERIOD_MS)) {
            last_measurement = now;
            if (!sensor_ready) {
                sht31_status_t status = sht31_init(&sensor, &sensor_config);
                sensor_ready = status == SHT31_STATUS_OK;
                if (!sensor_ready) {
                    report_measurement_failure("SHT31: inicializacao", status, display_ready);
                }
            }
            if (sensor_ready) {
                sht31_status_t status = sht31_start_measurement(&sensor);
                if (status == SHT31_STATUS_OK) {
                    // Capture time after the synchronous command has completed.
                    measurement_started = xTaskGetTickCount();
                    measurement_pending = true;
                } else {
                    report_measurement_failure("SHT31: comando de medicao", status, display_ready);
                }
            }
        }

        now = xTaskGetTickCount();
        if (measurement_pending && (TickType_t)(now - measurement_started) >= conversion_ticks) {
            sht31_sample_t sample;
            sht31_status_t status = sht31_read_measurement(&sensor, &sample);
            measurement_pending = false;
            if (status == SHT31_STATUS_OK) {
                update_measurement(&sample, display_ready);
            } else {
                report_measurement_failure("SHT31: leitura", status, display_ready);
            }
        }

        now = xTaskGetTickCount();
        if (display_ready && (!display_waiting_retry ||
                              (TickType_t)(now - last_display_failure) >= pdMS_TO_TICKS(MEASURE_PERIOD_MS))) {
            // Limit each iteration to one synchronous page; retry failures after 2 s.
            bool pending;
            ssd1306_status_t status = ssd1306_update(&display, &pending);
            display_waiting_retry = status != SSD1306_STATUS_OK;
            if (display_waiting_retry) {
                last_display_failure = xTaskGetTickCount();
                report_display_status("SSD1306: atualizacao", status);
            }
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
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
