/**
 * @file port_resources.c
 * @author Fabricio Santos (fmsantos8@ucs.br)
 * @brief Sequential startup resource ownership shared by all peripherals.
 *
 */
#include "port_resources.h"

#include "driver/gpio.h"
#include "framework_memory.h"

static uint64_t assigned_pins;

hal_status_t hal_esp32_check_startup(void)
{
    framework_memory_usage_t usage;
    hal_status_t status = framework_memory_get_usage(&usage);
    if (status != HAL_STATUS_OK) {
        return status;
    }
    return usage.sealed ? HAL_STATUS_INVALID_STATE : HAL_STATUS_OK;
}

hal_status_t hal_esp32_validate_pin(hal_pin_t pin, bool output)
{
    if (pin.port != 0 || !GPIO_IS_VALID_GPIO(pin.number)) {
        return HAL_STATUS_INVALID_ARGUMENT;
    }
    if ((pin.number >= 6 && pin.number <= 11) ||
        (output && !GPIO_IS_VALID_OUTPUT_GPIO(pin.number))) {
        return HAL_STATUS_NOT_SUPPORTED;
    }
    return HAL_STATUS_OK;
}

bool hal_esp32_pin_assigned(hal_pin_t pin)
{
    return (assigned_pins & (UINT64_C(1) << pin.number)) != 0;
}

void hal_esp32_claim_pin(hal_pin_t pin)
{
    assigned_pins |= UINT64_C(1) << pin.number;
}
