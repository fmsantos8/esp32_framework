/**
 * @file port_status.h
 * @author Fabricio Santos (fmsantos8@ucs.br)
 * @brief Private translation of SDK failures into common framework states.
 *
 */
#ifndef ESP32_PORT_STATUS_H
#define ESP32_PORT_STATUS_H

#include "esp_err.h"
#include "common/hal_status.h"

/**
 * @brief Translate an ESP-IDF operation result without exposing SDK codes.
 */
static inline hal_status_t hal_esp32_status(esp_err_t error)
{
    switch (error) {
    case ESP_OK: return HAL_STATUS_OK;
    case ESP_ERR_TIMEOUT: return HAL_STATUS_TIMEOUT;
    case ESP_ERR_INVALID_ARG: return HAL_STATUS_INVALID_ARGUMENT;
    case ESP_ERR_NOT_SUPPORTED: return HAL_STATUS_NOT_SUPPORTED;
    case ESP_ERR_NO_MEM: return HAL_STATUS_NO_MEMORY;
    case ESP_ERR_INVALID_STATE: return HAL_STATUS_INVALID_STATE;
    default: return HAL_STATUS_ERROR;
    }
}

#endif
