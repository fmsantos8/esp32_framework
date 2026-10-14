/**
 * @file test_contracts.c
 * @author Fabricio Santos (fmsantos8@ucs.br)
 * @brief Host checks of lifecycle, resource ownership, and transfer contracts.
 *
 */
#undef NDEBUG
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common/hal_pin.h"
#include "common/hal_contract.h"
#include "common/hal_port.h"
#include "common/hal_status.h"
#include "gpio/hal_gpio.h"
#include "i2c/hal_i2c.h"
#include "timer/hal_timer.h"
#include "uart/hal_uart.h"
#include "framework_memory.h"
#include "bsp_esp32.h"
#include "fake_sdk.h"

static void check_arena(void)
{
    void *memory = (void *)1;
    framework_memory_usage_t usage = {.capacity = 7};
    assert(framework_memory_get_usage(&usage) == HAL_STATUS_NOT_INITIALIZED);
    assert(usage.capacity == 7);
    assert(framework_memory_allocate(1, 1, &memory) == HAL_STATUS_NOT_INITIALIZED);
    assert(memory == NULL);
    assert(framework_memory_seal() == HAL_STATUS_NOT_INITIALIZED);
    assert(framework_memory_init() == HAL_STATUS_OK);
    assert(framework_memory_init() == HAL_STATUS_INVALID_STATE);
    assert(framework_memory_allocate(0, 1, &memory) == HAL_STATUS_INVALID_ARGUMENT);
    assert(framework_memory_allocate(1, 3, &memory) == HAL_STATUS_INVALID_ARGUMENT);
    assert(framework_memory_allocate(1, 0, &memory) == HAL_STATUS_INVALID_ARGUMENT);
    assert(framework_memory_allocate(1, 1, NULL) == HAL_STATUS_INVALID_ARGUMENT);
    assert(framework_memory_allocate(1, _Alignof(max_align_t) * 2, &memory) == HAL_STATUS_NOT_SUPPORTED);
    assert(framework_memory_allocate(3, 1, &memory) == HAL_STATUS_OK);
    memset(memory, 0x51, 3);
    void *first = memory;
    assert(framework_memory_allocate(8, 8, &memory) == HAL_STATUS_OK);
    assert((uintptr_t)memory % 8 == 0);
    assert(framework_memory_get_usage(&usage) == HAL_STATUS_OK);
    size_t before = usage.allocated;
    assert(framework_memory_allocate(SIZE_MAX, 8, &memory) == HAL_STATUS_NO_MEMORY);
    assert(memory == NULL);
    assert(framework_memory_get_usage(&usage) == HAL_STATUS_OK && usage.allocated == before);
    assert(framework_memory_allocate(usage.remaining, 1, &memory) == HAL_STATUS_OK);
    assert(framework_memory_allocate(1, 1, &memory) == HAL_STATUS_NO_MEMORY);
    assert(framework_memory_seal() == HAL_STATUS_OK);
    assert(framework_memory_seal() == HAL_STATUS_OK);
    assert(framework_memory_allocate(1, 1, &memory) == HAL_STATUS_INVALID_STATE);
    assert(framework_memory_init() == HAL_STATUS_INVALID_STATE);
    assert(*(unsigned char *)first == 0x51);
    assert(framework_memory_get_usage(&usage) == HAL_STATUS_OK && usage.sealed);
    assert(usage.capacity == 1024 && usage.remaining == 0);
}

static void check_ports(void)
{
    const hal_port_info_t *information;
    assert(hal_port_get_info(NULL) == HAL_STATUS_INVALID_ARGUMENT);
    assert(hal_port_get_info(&information) == HAL_STATUS_OK);
    assert(information->common_contract_version == HAL_CONTRACT_VERSION);
    assert(information->gpio_contract_version == HAL_GPIO_CONTRACT_VERSION);
    assert(information->i2c_contract_version == HAL_I2C_CONTRACT_VERSION);
    assert(information->uart_contract_version == HAL_UART_CONTRACT_VERSION);
    assert(information->timer_contract_version == HAL_TIMER_CONTRACT_VERSION);
    assert((information->i2c_capabilities & HAL_I2C_CAPABILITY_CONFIGURABLE_TIMEOUT) != 0);
    hal_gpio_config_t input_config = {{0, 18}, HAL_GPIO_INPUT, HAL_GPIO_PULL_NONE, false};
    hal_gpio_config_t output_config = {{0, 19}, HAL_GPIO_OUTPUT, HAL_GPIO_PULL_NONE, true};
    hal_gpio_t *input = (void *)1, *output = NULL, *duplicate = NULL;
    assert(hal_gpio_config(&input_config, &input) == HAL_STATUS_NOT_INITIALIZED && input == NULL);
    assert(framework_memory_init() == HAL_STATUS_OK);
    assert(hal_gpio_config(NULL, &input) == HAL_STATUS_INVALID_ARGUMENT && input == NULL);
    assert(hal_gpio_config(&input_config, NULL) == HAL_STATUS_INVALID_ARGUMENT);
    hal_gpio_config_t unsupported = input_config;
    unsupported.pin.port = 1;
    assert(hal_gpio_config(&unsupported, &duplicate) == HAL_STATUS_INVALID_ARGUMENT);
    unsupported.pin = (hal_pin_t){0, 6};
    assert(hal_gpio_config(&unsupported, &duplicate) == HAL_STATUS_NOT_SUPPORTED);
    unsupported.pin = (hal_pin_t){0, 34};
    unsupported.pull = HAL_GPIO_PULL_UP;
    assert(hal_gpio_config(&unsupported, &duplicate) == HAL_STATUS_NOT_SUPPORTED);
    unsupported.pull = HAL_GPIO_PULL_NONE;
    unsupported.direction = HAL_GPIO_OUTPUT;
    assert(hal_gpio_config(&unsupported, &duplicate) == HAL_STATUS_NOT_SUPPORTED);
    assert(hal_gpio_config(&input_config, &input) == HAL_STATUS_OK);
    assert(hal_gpio_config(&output_config, &output) == HAL_STATUS_OK);
    assert(fake_initial_level == 1);
    assert(hal_gpio_config(&input_config, &duplicate) == HAL_STATUS_RESOURCE_BUSY && duplicate == NULL);
    bool level = false;
    assert(hal_gpio_read(output, &level) == HAL_STATUS_NOT_SUPPORTED && !level);
    assert(hal_gpio_write(input, true) == HAL_STATUS_NOT_SUPPORTED);
    assert(hal_gpio_read(NULL, &level) == HAL_STATUS_INVALID_ARGUMENT && !level);
    assert(hal_gpio_read(input, &level) == HAL_STATUS_OK && level);
    assert(hal_gpio_write(output, false) == HAL_STATUS_OK && fake_initial_level == 0);

    const hal_timer_config_t timer_config = {0};
    hal_timer_t *timer, *other_timer;
    fake_time = 100000;
    assert(hal_timer_config(&timer_config, &timer) == HAL_STATUS_OK);
    assert(hal_timer_config(&timer_config, &other_timer) == HAL_STATUS_RESOURCE_BUSY && other_timer == NULL);
    uint32_t elapsed;
    fake_time += 123456;
    assert(hal_timer_read(timer, &elapsed) == HAL_STATUS_OK && elapsed == 123);
    fake_time += (INT64_C(1) << 32) * 1000;
    assert(hal_timer_read(timer, &elapsed) == HAL_STATUS_OK && elapsed == 123);

    hal_i2c_config_t bus_config = {0, {0, 22}, {0, 21}, 100000, 25};
    hal_i2c_t *bus, *other_bus;
    hal_i2c_config_t unsupported_timeout = bus_config;
    unsupported_timeout.timeout = 1;
    assert(hal_i2c_config(&unsupported_timeout, &other_bus) == HAL_STATUS_NOT_SUPPORTED);
    unsupported_timeout.timeout = HAL_TIMEOUT_MAXIMUM;
    assert(hal_i2c_config(&unsupported_timeout, &other_bus) == HAL_STATUS_NOT_SUPPORTED);
    assert(hal_i2c_config(&bus_config, &bus) == HAL_STATUS_OK);
    unsupported = input_config;
    unsupported.pin.number = 21;
    assert(hal_gpio_config(&unsupported, &duplicate) == HAL_STATUS_RESOURCE_BUSY);
    hal_i2c_config_t conflicting_bus = {0, {0, 25}, {0, 26}, 100000, 25};
    assert(hal_i2c_config(&conflicting_bus, &other_bus) == HAL_STATUS_RESOURCE_BUSY && other_bus == NULL);
    bus_config.instance = 1;
    bus_config.clock_pin.number = 19;
    assert(hal_i2c_config(&bus_config, &other_bus) == HAL_STATUS_RESOURCE_BUSY);
    bus_config.timeout = 0;
    assert(hal_i2c_config(&bus_config, &other_bus) == HAL_STATUS_INVALID_ARGUMENT);
    const uint8_t command[2] = {0x24, 0};
    uint8_t response[6];
    assert(hal_i2c_write(bus, 0x44, command, 2) == HAL_STATUS_OK);
    assert(fake_start_count == 1 && fake_stop_count == 1 && fake_addresses[0] == 0x88);
    assert(fake_last_timeout == 25);
    assert(hal_i2c_read(bus, 0x44, response, 6) == HAL_STATUS_OK && response[5] == 0x42);
    assert(fake_addresses[0] == 0x89 && fake_read_count == 2);
    assert(hal_i2c_read(bus, 0x44, response, 1) == HAL_STATUS_OK && fake_read_count == 1);
    assert(hal_i2c_write_read(bus, 0x44, command, 2, response, 6) == HAL_STATUS_OK);
    assert(fake_start_count == 2 && fake_stop_count == 1);
    assert(fake_addresses[0] == 0x88 && fake_addresses[1] == 0x89);
    size_t calls = fake_i2c_calls;
    assert(hal_i2c_write(bus, 0x07, command, 2) == HAL_STATUS_INVALID_ARGUMENT);
    assert(hal_i2c_write(bus, 0x78, command, 2) == HAL_STATUS_INVALID_ARGUMENT);
    assert(hal_i2c_write(bus, 0x44, command, 0) == HAL_STATUS_INVALID_ARGUMENT);
    assert(hal_i2c_write(bus, 0x44, command, 65536) == HAL_STATUS_NOT_SUPPORTED);
    assert(hal_i2c_write_read(bus, 0x44, response, 2, response + 1, 2) == HAL_STATUS_INVALID_ARGUMENT);
    assert(fake_i2c_calls == calls && fake_i2c_device_count == 1);
    fake_i2c_error = ESP_ERR_INVALID_RESPONSE;
    assert(hal_i2c_write(bus, 0x44, command, 2) == HAL_STATUS_ERROR);
    fake_i2c_error = ESP_ERR_TIMEOUT;
    assert(hal_i2c_write(bus, 0x44, command, 2) == HAL_STATUS_TIMEOUT);
    fake_i2c_error = ESP_OK;

    hal_uart_config_t uart_config = {1, {0, 16}, {0, 17}, 115200,
        HAL_UART_DATA_WIDTH_7, HAL_UART_PARITY_EVEN, HAL_UART_STOP_COUNT_2, 10};
    hal_uart_t *uart, *other_uart;
    hal_uart_config_t unsupported_baudrate = uart_config;
    unsupported_baudrate.baudrate = SOC_UART_BITRATE_MAX + 1;
    assert(hal_uart_config(&unsupported_baudrate, &other_uart) == HAL_STATUS_NOT_SUPPORTED);
    assert(hal_uart_config(&uart_config, &uart) == HAL_STATUS_OK);
    uart_config.instance = 2;
    uart_config.transmit_pin.number = 21;
    assert(hal_uart_config(&uart_config, &other_uart) == HAL_STATUS_RESOURCE_BUSY && other_uart == NULL);
    size_t count = 99;
    assert(hal_uart_write(NULL, command, 2, &count) == HAL_STATUS_INVALID_ARGUMENT && count == 0);
    assert(hal_uart_write(uart, NULL, 0, &count) == HAL_STATUS_OK && count == 0);
    const uint8_t invalid_payload[] = {1, 2, 0xff};
    assert(hal_uart_write(uart, invalid_payload, sizeof(invalid_payload), &count) == HAL_STATUS_INVALID_ARGUMENT);
    assert(count == 0 && fake_uart_accepted == 0);
    uint8_t payload[300] = {0};
    fake_uart_wait_duration = 1000;
    assert(hal_uart_write(uart, payload, sizeof(payload), &count) == HAL_STATUS_OK && count == 300);
    fake_uart_chunk = 4;
    fake_uart_wait_error = ESP_ERR_TIMEOUT;
    assert(hal_uart_write(uart, payload, sizeof(payload), &count) == HAL_STATUS_TIMEOUT && count == 4);
    fake_uart_wait_error = ESP_OK;
    fake_uart_chunk = 128;
    fake_uart_wait_duration = 6000;
    assert(hal_uart_write(uart, payload, sizeof(payload), &count) == HAL_STATUS_TIMEOUT && count == 256);

    assert(framework_memory_seal() == HAL_STATUS_OK);
    assert(hal_gpio_config(&input_config, &duplicate) == HAL_STATUS_INVALID_STATE && duplicate == NULL);
    assert(hal_gpio_write(output, true) == HAL_STATUS_OK);
    assert(hal_i2c_write(bus, 0x44, command, 2) == HAL_STATUS_OK);
}

static void check_exhaustion(void)
{
    assert(framework_memory_init() == HAL_STATUS_OK);
    framework_memory_usage_t usage;
    assert(framework_memory_get_usage(&usage) == HAL_STATUS_OK);
    void *memory;
    assert(framework_memory_allocate(usage.capacity, 1, &memory) == HAL_STATUS_OK);
    const hal_gpio_config_t config = {{0, 18}, HAL_GPIO_INPUT, HAL_GPIO_PULL_NONE, false};
    hal_gpio_t *gpio = (void *)1;
    assert(hal_gpio_config(&config, &gpio) == HAL_STATUS_NO_MEMORY && gpio == NULL);
}

static void check_failed_construction(void)
{
    assert(framework_memory_init() == HAL_STATUS_OK);
    const hal_gpio_config_t config = {{0, 19}, HAL_GPIO_OUTPUT, HAL_GPIO_PULL_NONE, false};
    hal_gpio_t *gpio = (void *)1;
    fake_gpio_error = ESP_FAIL;
    assert(hal_gpio_config(&config, &gpio) == HAL_STATUS_ERROR && gpio == NULL);
    framework_memory_usage_t usage;
    assert(framework_memory_get_usage(&usage) == HAL_STATUS_OK && usage.allocated > 0);
    fake_gpio_error = ESP_OK;
    assert(hal_gpio_config(&config, &gpio) == HAL_STATUS_OK);
    const hal_i2c_config_t bus_config = {0, {0, 22}, {0, 21}, 100000, 1000};
    hal_i2c_t *bus;
    fake_i2c_error = ESP_ERR_NO_MEM;
    assert(hal_i2c_config(&bus_config, &bus) == HAL_STATUS_NO_MEMORY && bus == NULL);
}

static void check_bsp(void)
{
    assert(bsp_esp32_get_led() == NULL && bsp_esp32_get_i2c() == NULL);
    assert(framework_memory_init() == HAL_STATUS_OK);
    assert(bsp_esp32_init() == HAL_STATUS_OK);
    assert(fake_initial_level == 0);
    assert(bsp_esp32_get_button() && bsp_esp32_get_led() && bsp_esp32_get_i2c());
    assert(bsp_esp32_get_uart() && bsp_esp32_get_timer());
    assert(!bsp_esp32_get_button_active_level() && bsp_esp32_get_led_active_level());
    assert(bsp_esp32_get_sht31_address() == 0x44 && bsp_esp32_get_ssd1306_address() == 0x3c);
    assert(bsp_esp32_init() == HAL_STATUS_INVALID_STATE);
    assert(framework_memory_seal() == HAL_STATUS_OK);
}

int main(int argc, char **argv)
{
    assert(argc == 2);
    if (strcmp(argv[1], "arena") == 0) check_arena();
    else if (strcmp(argv[1], "ports") == 0) check_ports();
    else if (strcmp(argv[1], "exhaustion") == 0) check_exhaustion();
    else if (strcmp(argv[1], "failed-construction") == 0) check_failed_construction();
    else if (strcmp(argv[1], "bsp") == 0) check_bsp();
    else return 2;
    printf("PASS: %s\n", argv[1]);
    return 0;
}
