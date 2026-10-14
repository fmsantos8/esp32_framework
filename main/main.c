/**
 * @file main.c
 * @author Fabricio Santos (fmsantos8@ucs.br)
 * @brief Existing hardware smoke test adapted to the constructor-based HAL.
 *
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "common/hal_contract.h"
#include "common/hal_port.h"
#include "common/hal_status.h"
#include "gpio/hal_gpio.h"
#include "i2c/hal_i2c.h"
#include "timer/hal_timer.h"
#include "uart/hal_uart.h"
#include "bsp_esp32.h"
#include "framework_memory.h"

enum {
    OLED_WIDTH = 128,
    OLED_PAGES = 64 / 8,
};
#define OLED_LINE_CHARACTERS   10
/**
 * @brief Sensor acquisition interval, in milliseconds.
 */
#define MEASURE_PERIOD 2000

// Only the characters used by this visual test are needed (5x7, bit 0 at top).
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

static bool report_status(const char *step, hal_status_t status) {
    printf("%s: %s (status %d)\n", step, status == HAL_STATUS_OK ? "OK" : "FALHOU", status);
    return status == HAL_STATUS_OK;
}

/* Electrical levels and I2C addresses of the current test assembly. */
static bool button_active_level(void) {
    return bsp_esp32_get_button_active_level();
}

static bool led_active_level(void) {
    return bsp_esp32_get_led_active_level();
}

static uint8_t sht31_address(void) {
    return bsp_esp32_get_sht31_address();
}

static uint8_t ssd1306_address(void) {
    return bsp_esp32_get_ssd1306_address();
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

/**
 * @brief Acquire temperature and humidity with the existing blocking test flow.
 *
 * @param[out] temperature Temperature in tenths of a degree Celsius, only on success.
 * @param[out] humidity Relative humidity in tenths of a percent, only on success.
 *
 * @return True for a complete sample with valid checksums.
 */
static bool sht31_measure(int32_t *temperature, uint32_t *humidity) {
    // Single-shot measurement, high repeatability, without clock stretching.
    const uint8_t command[] = {0x24, 0x00};
    hal_status_t status = hal_i2c_write(bsp_esp32_get_i2c(), sht31_address(), command, sizeof(command));
    if (status != HAL_STATUS_OK) {
        report_status("SHT31: comando de medicao", status);
        return false;
    }

    vTaskDelay(pdMS_TO_TICKS(30));
    uint8_t sample[6];
    status = hal_i2c_read(bsp_esp32_get_i2c(), sht31_address(), sample, sizeof(sample));
    if (status != HAL_STATUS_OK) {
        report_status("SHT31: leitura", status);
        return false;
    }
    if (sht31_crc(sample) != sample[2] ||
        sht31_crc(sample + 3) != sample[5]) {
        printf("SHT31: CRC FALHOU\n");
        return false;
    }

    uint16_t raw_temperature = ((uint16_t)sample[0] << 8) | sample[1];
    uint16_t raw_humidity = ((uint16_t)sample[3] << 8) | sample[4];
    *temperature = -450 + (int32_t)((1750UL * raw_temperature + 32767UL) / 65535UL);
    *humidity = (1000UL * raw_humidity + 32767UL) / 65535UL;
    return true;
}

static hal_status_t oled_write_page(uint8_t page, const uint8_t data[OLED_WIDTH + 1]) {
    const uint8_t address[] = {0x00, (uint8_t)(0xB0 | page), 0x00, 0x10};
    hal_status_t status = hal_i2c_write(bsp_esp32_get_i2c(), ssd1306_address(), address, sizeof(address));
    if (status != HAL_STATUS_OK) {
        return status;
    }
    return hal_i2c_write(bsp_esp32_get_i2c(), ssd1306_address(), data, OLED_WIDTH + 1);
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
    hal_status_t status = hal_i2c_write(bsp_esp32_get_i2c(), ssd1306_address(), init, sizeof(init));
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
    return hal_i2c_write(bsp_esp32_get_i2c(), ssd1306_address(), display_on, sizeof(display_on));
}

static hal_status_t oled_write_line(uint8_t first_page, const char *text) {
    uint8_t top[OLED_WIDTH + 1] = {0x40};
    uint8_t bottom[OLED_WIDTH + 1] = {0x40};
    for (uint8_t character = 0;
         character < OLED_LINE_CHARACTERS && text[character] != '\0';
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
    printf("SHT31: %s%ld.%ld C, %lu.%lu %% UR\n",
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
    const hal_port_info_t *information = NULL;
    if (!report_status("HAL: informacoes do porte", hal_port_get_info(&information))) {
        return;
    }
    if (information->port_contract_version != HAL_PORT_CONTRACT_VERSION ||
        information->common_contract_version != HAL_CONTRACT_VERSION ||
        information->gpio_contract_version != HAL_GPIO_CONTRACT_VERSION ||
        information->i2c_contract_version != HAL_I2C_CONTRACT_VERSION ||
        information->uart_contract_version != HAL_UART_CONTRACT_VERSION ||
        information->timer_contract_version != HAL_TIMER_CONTRACT_VERSION ||
        (information->gpio_capabilities & (HAL_GPIO_CAPABILITY_READ | HAL_GPIO_CAPABILITY_WRITE)) !=
            (HAL_GPIO_CAPABILITY_READ | HAL_GPIO_CAPABILITY_WRITE) ||
        (information->i2c_capabilities & (HAL_I2C_CAPABILITY_WRITE | HAL_I2C_CAPABILITY_READ |
                                        HAL_I2C_CAPABILITY_WRITE_READ)) !=
            (HAL_I2C_CAPABILITY_WRITE | HAL_I2C_CAPABILITY_READ | HAL_I2C_CAPABILITY_WRITE_READ) ||
        (information->uart_capabilities & HAL_UART_CAPABILITY_WRITE) == 0 ||
        (information->timer_capabilities & HAL_TIMER_CAPABILITY_READ) == 0) {
        report_status("HAL: contratos e capacidades", HAL_STATUS_NOT_SUPPORTED);
        return;
    }
    if (!report_status("Memoria: inicializacao", framework_memory_init()) ||
        !report_status("BSP: inicializacao", bsp_esp32_init()) ||
        !report_status("Memoria: encerrar alocacoes", framework_memory_seal())) {
        return;
    }
    framework_memory_usage_t usage;
    if (report_status("Memoria: consulta de uso", framework_memory_get_usage(&usage))) {
        printf("Arena HAL: %u/%u bytes utilizados\n",
               (unsigned)usage.allocated, (unsigned)usage.capacity);
    }

    static const uint8_t message[] = "UART0: transmissao pelo port OK\r\n";
    size_t transmitted_count = 0;
    report_status("UART0: transmissao",
                  hal_uart_write(bsp_esp32_get_uart(), message, sizeof(message) - 1,
                                 &transmitted_count));
    printf("UART0: %u bytes aceitos\n", (unsigned)transmitted_count);

    uint32_t before;
    uint32_t after;
    if (report_status("Timer: primeira leitura",
                      hal_timer_read(bsp_esp32_get_timer(), &before))) {
        vTaskDelay(pdMS_TO_TICKS(30));
        if (report_status("Timer: segunda leitura",
                          hal_timer_read(bsp_esp32_get_timer(), &after))) {
            printf("Timer: %s (%lu ms decorridos)\n",
                   (uint32_t)(after - before) >= 10 ? "OK" : "FALHOU",
                   (unsigned long)(uint32_t)(after - before));
        }
    }

    bool button_ready = true;
    bool led_ready = report_status("GPIO19: acender LED",
                                   hal_gpio_write(bsp_esp32_get_led(),
                                                  led_active_level()));
    vTaskDelay(pdMS_TO_TICKS(200));
    led_ready = report_status("GPIO19: apagar LED",
                              hal_gpio_write(bsp_esp32_get_led(),
                                             !led_active_level())) &&
                led_ready;

    bool display_ready = report_status("SSD1306: inicializacao", oled_init());

    printf("Pressione o botao: o LED deve acender; solte para apagar.\n");
    bool last_pressed = false;
    bool has_sample = false;
    TickType_t last_measurement = xTaskGetTickCount() - pdMS_TO_TICKS(MEASURE_PERIOD);
    while (true) {
        TickType_t now = xTaskGetTickCount();
        if ((TickType_t)(now - last_measurement) >=
            pdMS_TO_TICKS(MEASURE_PERIOD)) {
            last_measurement = now;
            update_measurement(display_ready);
        }

        if (button_ready) {
            bool level;
            hal_status_t status = hal_gpio_read(bsp_esp32_get_button(), &level);
            if (status != HAL_STATUS_OK) {
                report_status("GPIO18: leitura", status);
                button_ready = false;
            } else {
                bool pressed = level == button_active_level();
                if (!has_sample || pressed != last_pressed) {
                    printf("Botao: %s\n", pressed ? "pressionado" : "solto");
                    last_pressed = pressed;
                    has_sample = true;
                }
                if (led_ready) {
                    status = hal_gpio_write(bsp_esp32_get_led(), pressed ? led_active_level() : !led_active_level());
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
