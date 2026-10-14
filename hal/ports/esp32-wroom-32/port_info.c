/**
 * @file port_info.c
 * @author Fabricio Santos (fmsantos8@ucs.br)
 * @brief Immutable contract versions and ESP32 capability declarations.
 *
 */
#include "common/hal_contract.h"
#include "common/hal_port.h"
#include "common/hal_status.h"
#include "gpio/hal_gpio.h"
#include "i2c/hal_i2c.h"
#include "timer/hal_timer.h"
#include "uart/hal_uart.h"

#include <stddef.h>

_Static_assert(HAL_CONTRACT_VERSION == 2u && HAL_PORT_CONTRACT_VERSION == 1u &&
               HAL_GPIO_CONTRACT_VERSION == 1u && HAL_I2C_CONTRACT_VERSION == 2u &&
               HAL_UART_CONTRACT_VERSION == 2u && HAL_TIMER_CONTRACT_VERSION == 1u,
               "Audit this port when changing public contracts.");

static const hal_port_info_t information = {
    .name = "esp32-wroom-32 / ESP-IDF",
    .port_contract_version = 1,
    .common_contract_version = 2,
    .gpio_contract_version = 1,
    .i2c_contract_version = 2,
    .uart_contract_version = 2,
    .timer_contract_version = 1,
    .gpio_capabilities = HAL_GPIO_CAPABILITY_READ | HAL_GPIO_CAPABILITY_WRITE |
                         HAL_GPIO_CAPABILITY_PULL_UP | HAL_GPIO_CAPABILITY_PULL_DOWN,
    .i2c_capabilities = HAL_I2C_CAPABILITY_WRITE | HAL_I2C_CAPABILITY_READ |
                        HAL_I2C_CAPABILITY_WRITE_READ | HAL_I2C_CAPABILITY_CONFIGURABLE_TIMEOUT,
    .uart_capabilities = HAL_UART_CAPABILITY_WRITE | HAL_UART_CAPABILITY_DATA_WIDTH_7 |
                         HAL_UART_CAPABILITY_DATA_WIDTH_8 | HAL_UART_CAPABILITY_PARITY_NONE |
                         HAL_UART_CAPABILITY_PARITY_EVEN | HAL_UART_CAPABILITY_PARITY_ODD |
                         HAL_UART_CAPABILITY_STOP_COUNT_1 | HAL_UART_CAPABILITY_STOP_COUNT_2 |
                         HAL_UART_CAPABILITY_CONFIGURABLE_TIMEOUT,
    .timer_capabilities = HAL_TIMER_CAPABILITY_READ,
};

hal_status_t hal_port_get_info(const hal_port_info_t **out_information)
{
    if (out_information == NULL) {
        return HAL_STATUS_INVALID_ARGUMENT;
    }
    *out_information = &information;
    return HAL_STATUS_OK;
}
