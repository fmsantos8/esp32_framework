#ifndef ESP32_PORT_STATUS_H
#define ESP32_PORT_STATUS_H

#include "esp_err.h"
#include "status/hal_status.h"

static inline hal_status_t hal_esp32_status(esp_err_t error)
{
    switch (error) {
    case ESP_OK:
        return HAL_STATUS_OK;
    case ESP_ERR_TIMEOUT:
        return HAL_STATUS_TIMEOUT;
    case ESP_ERR_INVALID_ARG:
        return HAL_STATUS_INVALID_PARAM;
    case ESP_ERR_NOT_SUPPORTED:
        return HAL_STATUS_NOT_SUPPORTED;
    default:
        return HAL_STATUS_ERROR;
    }
}

#endif
