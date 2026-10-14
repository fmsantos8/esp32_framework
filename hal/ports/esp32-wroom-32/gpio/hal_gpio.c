/**
 * @file hal_gpio.c
 * @author Fabricio Santos (fmsantos8@ucs.br)
 * @brief ESP-IDF GPIO constructors and physical-level operations.
 *
 */
#include "gpio/hal_gpio_esp32.h"
#include "port_resources.h"
#include "port_status.h"
#include "framework_memory.h"

#include <string.h>

hal_status_t hal_gpio_config(const hal_gpio_config_t *config, hal_gpio_t **out_gpio)
{
    if (out_gpio == NULL) {
        return HAL_STATUS_INVALID_ARGUMENT;
    }
    *out_gpio = NULL;
    if (config == NULL ||
        (config->direction != HAL_GPIO_INPUT && config->direction != HAL_GPIO_OUTPUT) ||
        (config->pull != HAL_GPIO_PULL_NONE && config->pull != HAL_GPIO_PULL_UP &&
         config->pull != HAL_GPIO_PULL_DOWN)) {
        return HAL_STATUS_INVALID_ARGUMENT;
    }
    hal_status_t status = hal_esp32_validate_pin(config->pin, config->direction == HAL_GPIO_OUTPUT);
    if (status != HAL_STATUS_OK) {
        return status;
    }
    if (config->pin.number >= 34 && config->pull != HAL_GPIO_PULL_NONE) {
        return HAL_STATUS_NOT_SUPPORTED;
    }
    status = hal_esp32_check_startup();
    if (status != HAL_STATUS_OK) {
        return status;
    }
    if (hal_esp32_pin_assigned(config->pin)) {
        return HAL_STATUS_RESOURCE_BUSY;
    }
    void *storage = NULL;
    status = framework_memory_allocate(sizeof(hal_gpio_t), _Alignof(hal_gpio_t), &storage);
    if (status != HAL_STATUS_OK) {
        return status;
    }
    if (config->direction == HAL_GPIO_OUTPUT) {
        status = hal_esp32_status(gpio_set_level(config->pin.number, config->initial_level));
        if (status != HAL_STATUS_OK) {
            return status;
        }
    }
    const gpio_config_t driver_config = {
        .pin_bit_mask = UINT64_C(1) << config->pin.number,
        .mode = config->direction == HAL_GPIO_INPUT ? GPIO_MODE_INPUT : GPIO_MODE_OUTPUT,
        .pull_up_en = config->pull == HAL_GPIO_PULL_UP ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE,
        .pull_down_en = config->pull == HAL_GPIO_PULL_DOWN ? GPIO_PULLDOWN_ENABLE : GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    status = hal_esp32_status(gpio_config(&driver_config));
    if (status != HAL_STATUS_OK) {
        return status;
    }
    const hal_gpio_t object = {
        .pin = config->pin.number,
        .direction = config->direction,
        .configured = true,
    };
    memcpy(storage, &object, sizeof(object));
    hal_esp32_claim_pin(config->pin);
    *out_gpio = storage;
    return HAL_STATUS_OK;
}

hal_status_t hal_gpio_read(hal_gpio_t *gpio, bool *out_level)
{
    if (gpio == NULL || out_level == NULL) {
        return HAL_STATUS_INVALID_ARGUMENT;
    }
    hal_gpio_t object;
    memcpy(&object, gpio, sizeof(object));
    if (!object.configured) {
        return HAL_STATUS_NOT_INITIALIZED;
    }
    if (object.direction != HAL_GPIO_INPUT) {
        return HAL_STATUS_NOT_SUPPORTED;
    }
    *out_level = gpio_get_level(object.pin) != 0;
    return HAL_STATUS_OK;
}

hal_status_t hal_gpio_write(hal_gpio_t *gpio, bool level)
{
    if (gpio == NULL) {
        return HAL_STATUS_INVALID_ARGUMENT;
    }
    hal_gpio_t object;
    memcpy(&object, gpio, sizeof(object));
    if (!object.configured) {
        return HAL_STATUS_NOT_INITIALIZED;
    }
    if (object.direction != HAL_GPIO_OUTPUT) {
        return HAL_STATUS_NOT_SUPPORTED;
    }
    return hal_esp32_status(gpio_set_level(object.pin, level));
}
