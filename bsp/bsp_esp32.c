/**
 * @file bsp_esp32.c
 * @author Fabricio Santos (fmsantos8@ucs.br)
 * @brief Physical resources of the existing ESP32 test assembly.
 *
 */
#include "bsp_esp32.h"

static hal_gpio_t *button;
static hal_gpio_t *led;
static hal_i2c_t *i2c;
static hal_uart_t *uart;
static hal_timer_t *timer;
static bool initialization_started;

hal_status_t bsp_esp32_init(void)
{
    if (initialization_started) {
        return HAL_STATUS_INVALID_STATE;
    }
    initialization_started = true;
    const hal_gpio_config_t led_config = {
        .pin = {.port = 0, .number = 19},
        .direction = HAL_GPIO_OUTPUT,
        .pull = HAL_GPIO_PULL_NONE,
        .initial_level = false,
    };
    hal_status_t status = hal_gpio_config(&led_config, &led);
    if (status != HAL_STATUS_OK) {
        return status;
    }
    const hal_gpio_config_t button_config = {
        .pin = {.port = 0, .number = 18},
        .direction = HAL_GPIO_INPUT,
        .pull = HAL_GPIO_PULL_NONE,
        .initial_level = false,
    };
    status = hal_gpio_config(&button_config, &button);
    if (status != HAL_STATUS_OK) {
        return status;
    }
    const hal_i2c_config_t i2c_config = {
        .instance = 0,
        .clock_pin = {.port = 0, .number = 22},
        .data_pin = {.port = 0, .number = 21},
        .frequency = 100000,
        .timeout = 1000,
    };
    status = hal_i2c_config(&i2c_config, &i2c);
    if (status != HAL_STATUS_OK) {
        return status;
    }
    const hal_uart_config_t uart_config = {
        .instance = 0,
        .transmit_pin = {.port = 0, .number = 1},
        .receive_pin = {.port = 0, .number = 3},
        .baudrate = 115200,
        .data_width = HAL_UART_DATA_WIDTH_8,
        .parity = HAL_UART_PARITY_NONE,
        .stop_count = HAL_UART_STOP_COUNT_1,
        .timeout = 1000,
    };
    status = hal_uart_config(&uart_config, &uart);
    if (status != HAL_STATUS_OK) {
        return status;
    }
    const hal_timer_config_t timer_config = {.instance = 0};
    return hal_timer_config(&timer_config, &timer);
}

hal_gpio_t *bsp_esp32_get_button(void) { return button; }
hal_gpio_t *bsp_esp32_get_led(void) { return led; }
hal_i2c_t *bsp_esp32_get_i2c(void) { return i2c; }
hal_uart_t *bsp_esp32_get_uart(void) { return uart; }
hal_timer_t *bsp_esp32_get_timer(void) { return timer; }
bool bsp_esp32_get_button_active_level(void) { return false; }
bool bsp_esp32_get_led_active_level(void) { return true; }
uint8_t bsp_esp32_get_sht31_address(void) { return 0x44; }
uint8_t bsp_esp32_get_ssd1306_address(void) { return 0x3c; }
