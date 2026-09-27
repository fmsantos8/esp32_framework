#include "hal_gpio_esp32.h"

static bool is_valid_config(const hal_gpio_config_t *config) {
    return config != NULL &&
           (config->direction == HAL_GPIO_DIRECTION_INPUT ||
            config->direction == HAL_GPIO_DIRECTION_OUTPUT) &&
           (config->pull == HAL_GPIO_PULL_NONE ||
            config->pull == HAL_GPIO_PULL_UP ||
            config->pull == HAL_GPIO_PULL_DOWN);
}

static gpio_mode_t get_direction_mode(hal_gpio_direction_t direction) {
    switch (direction) {
        case HAL_GPIO_DIRECTION_INPUT:
            return GPIO_MODE_INPUT;
        case HAL_GPIO_DIRECTION_OUTPUT:
            return GPIO_MODE_OUTPUT;
        default:
            return GPIO_MODE_DISABLE; // Invalid direction
    }
}

static gpio_pullup_t get_pullup_mode(hal_gpio_pull_t pull) {
    switch (pull) {
        case HAL_GPIO_PULL_NONE:
            return GPIO_PULLUP_DISABLE;
        case HAL_GPIO_PULL_UP:
            return GPIO_PULLUP_ENABLE;
        case HAL_GPIO_PULL_DOWN:
            return GPIO_PULLUP_DISABLE; // Pull-down is handled separately
        default:
            return GPIO_PULLUP_DISABLE; // Invalid pull
    }
}

static gpio_pulldown_t get_pulldown_mode(hal_gpio_pull_t pull) {
    switch (pull) {
        case HAL_GPIO_PULL_NONE:
            return GPIO_PULLDOWN_DISABLE;
        case HAL_GPIO_PULL_UP:
            return GPIO_PULLDOWN_DISABLE; // Pull-up is handled separately
        case HAL_GPIO_PULL_DOWN:
            return GPIO_PULLDOWN_ENABLE;
        default:
            return GPIO_PULLDOWN_DISABLE; // Invalid pull
    }
}

hal_status_t hal_gpio_config(hal_gpio_t *gpio, const hal_gpio_config_t *config) {

    if (gpio == NULL || !is_valid_config(config)) {
        return HAL_STATUS_INVALID_PARAM;
    }

    if (!GPIO_IS_VALID_GPIO(gpio->pin) ||
        (config->direction == HAL_GPIO_DIRECTION_OUTPUT &&
         !GPIO_IS_VALID_OUTPUT_GPIO(gpio->pin))) {
        return HAL_STATUS_NOT_SUPPORTED;
    }

    if (config->direction == HAL_GPIO_DIRECTION_OUTPUT) {
        esp_err_t err = gpio_set_level(gpio->pin, 0);
        if (err != ESP_OK) {
            return hal_esp32_status(err);
        }
    }

    gpio_config_t driver_config = {
        .pin_bit_mask = 1ULL << gpio->pin,
        .mode = get_direction_mode(config->direction),
        .pull_up_en = get_pullup_mode(config->pull),
        .pull_down_en = get_pulldown_mode(config->pull),
        .intr_type = GPIO_INTR_DISABLE,
    };

    esp_err_t err = gpio_config(&driver_config);
    if (err != ESP_OK) {
        return hal_esp32_status(err);
    }
    gpio->direction = config->direction;
    gpio->configured = true;

    return HAL_STATUS_OK;
}

hal_status_t hal_gpio_read(hal_gpio_t *gpio, bool *level) {
    if (gpio == NULL || level == NULL) {
        return HAL_STATUS_INVALID_PARAM;
    }
    if (!gpio->configured) {
        return HAL_STATUS_ERROR;
    }
    *level = gpio_get_level(gpio->pin) != 0;
    return HAL_STATUS_OK;
}

hal_status_t hal_gpio_write(hal_gpio_t *gpio, bool level) {
    if (gpio == NULL) {
        return HAL_STATUS_INVALID_PARAM;
    }
    if (!gpio->configured || gpio->direction != HAL_GPIO_DIRECTION_OUTPUT) {
        return HAL_STATUS_ERROR;
    }
    return hal_esp32_status(gpio_set_level(gpio->pin, level ? 1 : 0));
}
