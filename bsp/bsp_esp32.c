#include "bsp_esp32.h"

#include "gpio/hal_gpio_esp32.h"
#include "i2c/hal_i2c_esp32.h"
#include "timer/hal_timer_esp32.h"
#include "uart/hal_uart_esp32.h"

static hal_gpio_t button = {.pin = GPIO_NUM_18};
static hal_gpio_t led = {.pin = GPIO_NUM_19};
static hal_i2c_t i2c = {
    .controller = I2C_NUM_0,
    .sda_pin = GPIO_NUM_21,
    .scl_pin = GPIO_NUM_22,
};
static hal_uart_t uart = {
    .port = UART_NUM_0,
    .tx_pin = GPIO_NUM_1,
    .rx_pin = GPIO_NUM_3,
};
static hal_timer_t timer;

static hal_status_t init_button(void) {
    const hal_gpio_config_t config = {
        .direction = HAL_GPIO_DIRECTION_INPUT,
        .pull = HAL_GPIO_PULL_NONE, // External 10 kOhm pull-up.
    };
    return hal_gpio_config(&button, &config);
}

static hal_status_t init_led(void) {
    const hal_gpio_config_t config = {
        .direction = HAL_GPIO_DIRECTION_OUTPUT,
        .pull = HAL_GPIO_PULL_NONE,
    };
    // The HAL drives this active-high LED low during configuration.
    return hal_gpio_config(&led, &config);
}

static hal_status_t init_i2c(void) {
    const hal_i2c_config_t config = {.clock_speed = 100000};
    return hal_i2c_config(&i2c, &config);
}

static hal_status_t init_uart(void) {
    const hal_uart_config_t config = {
        .baud_rate = 115200,
        .parity = HAL_UART_PARITY_NONE,
        .stop_bits = HAL_UART_STOP_BITS_1,
    };
    return hal_uart_config(&uart, &config);
}

void bsp_esp32_init(void) {
    assert(init_button() == HAL_STATUS_OK);
    assert(init_led() == HAL_STATUS_OK);
    assert(init_i2c() == HAL_STATUS_OK);
    assert(init_uart() == HAL_STATUS_OK);
    assert(hal_timer_start(&timer) == HAL_STATUS_OK);
}

hal_gpio_t *bsp_esp32_get_button(void) {
    return &button;
}

hal_gpio_t *bsp_esp32_get_led(void) {
    return &led;
}

hal_i2c_t *bsp_esp32_get_i2c(void) {
    return &i2c;
}

hal_uart_t *bsp_esp32_get_uart(void) {
    return &uart;
}

hal_timer_t *bsp_esp32_get_timer(void) {
    return &timer;
}

bool bsp_esp32_get_button_active_level(void) {
    return false;
}

bool bsp_esp32_get_led_active_level(void) {
    return true;
}

uint8_t bsp_esp32_get_sht31_address(void) {
    return 0x44;
}

uint8_t bsp_esp32_get_ssd1306_address(void) {
    return 0x3C;
}
