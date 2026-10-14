/**
 * @file hal_uart.c
 * @author Fabricio Santos (fmsantos8@ucs.br)
 * @brief Bounded FIFO transmission with a single monotonic deadline.
 *
 */
#include "uart/hal_uart_esp32.h"
#include "port_resources.h"
#include "port_status.h"
#include "framework_memory.h"

#include <string.h>
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "soc/soc_caps.h"

static bool assigned_controllers[UART_NUM_MAX];

/**
 * @brief Convert remaining microseconds to finite scheduler ticks, rounding up.
 */
static TickType_t remaining_ticks(int64_t remaining)
{
    uint64_t ticks = ((uint64_t)remaining * configTICK_RATE_HZ + 999999) / 1000000;
    if (ticks >= portMAX_DELAY) {
        ticks = portMAX_DELAY - 1;
    }
    return (TickType_t)ticks;
}

hal_status_t hal_uart_config(const hal_uart_config_t *config, hal_uart_t **out_uart)
{
    if (out_uart == NULL) {
        return HAL_STATUS_INVALID_ARGUMENT;
    }
    *out_uart = NULL;
    if (config == NULL || config->instance >= UART_NUM_MAX || config->baudrate == 0 ||
        config->timeout == 0 || config->timeout > HAL_TIMEOUT_MAXIMUM ||
        (config->data_width != HAL_UART_DATA_WIDTH_7 && config->data_width != HAL_UART_DATA_WIDTH_8) ||
        (config->parity != HAL_UART_PARITY_NONE && config->parity != HAL_UART_PARITY_EVEN &&
         config->parity != HAL_UART_PARITY_ODD) ||
        (config->stop_count != HAL_UART_STOP_COUNT_1 && config->stop_count != HAL_UART_STOP_COUNT_2) ||
        (config->transmit_pin.port == config->receive_pin.port &&
         config->transmit_pin.number == config->receive_pin.number)) {
        return HAL_STATUS_INVALID_ARGUMENT;
    }
    hal_status_t status = hal_esp32_validate_pin(config->transmit_pin, true);
    if (status != HAL_STATUS_OK) {
        return status;
    }
    status = hal_esp32_validate_pin(config->receive_pin, false);
    if (status != HAL_STATUS_OK) {
        return status;
    }
    if (config->baudrate > SOC_UART_BITRATE_MAX) {
        return HAL_STATUS_NOT_SUPPORTED;
    }
    status = hal_esp32_check_startup();
    if (status != HAL_STATUS_OK) {
        return status;
    }
    if (assigned_controllers[config->instance] || uart_is_driver_installed(config->instance) ||
        hal_esp32_pin_assigned(config->transmit_pin) || hal_esp32_pin_assigned(config->receive_pin)) {
        return HAL_STATUS_RESOURCE_BUSY;
    }
    void *storage = NULL;
    status = framework_memory_allocate(sizeof(hal_uart_t), _Alignof(hal_uart_t), &storage);
    if (status != HAL_STATUS_OK) {
        return status;
    }
    const uart_config_t driver_config = {
        .baud_rate = (int)config->baudrate,
        .data_bits = config->data_width == HAL_UART_DATA_WIDTH_7 ? UART_DATA_7_BITS : UART_DATA_8_BITS,
        .parity = config->parity == HAL_UART_PARITY_NONE ? UART_PARITY_DISABLE :
                  config->parity == HAL_UART_PARITY_EVEN ? UART_PARITY_EVEN : UART_PARITY_ODD,
        .stop_bits = config->stop_count == HAL_UART_STOP_COUNT_1 ? UART_STOP_BITS_1 : UART_STOP_BITS_2,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    /* No TX ring buffer: the SDK copies accepted bytes straight into its FIFO. */
    esp_err_t error = uart_driver_install(config->instance, 256, 0, 0, NULL, 0);
    if (error != ESP_OK) {
        return hal_esp32_status(error);
    }
    error = uart_param_config(config->instance, &driver_config);
    const bool unsupported_configuration = error == ESP_FAIL || error == ESP_ERR_NOT_SUPPORTED;
    if (error == ESP_OK) {
        error = uart_set_pin(config->instance, config->transmit_pin.number,
                             config->receive_pin.number, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    }
    if (error != ESP_OK) {
        if (uart_driver_delete(config->instance) != ESP_OK) {
            assigned_controllers[config->instance] = true;
            hal_esp32_claim_pin(config->transmit_pin);
            hal_esp32_claim_pin(config->receive_pin);
        }
        return unsupported_configuration ? HAL_STATUS_NOT_SUPPORTED : hal_esp32_status(error);
    }
    const hal_uart_t object = {
        .controller = config->instance,
        .data_width = config->data_width,
        .configured = true,
        .timeout = config->timeout,
    };
    memcpy(storage, &object, sizeof(object));
    assigned_controllers[config->instance] = true;
    hal_esp32_claim_pin(config->transmit_pin);
    hal_esp32_claim_pin(config->receive_pin);
    *out_uart = storage;
    return HAL_STATUS_OK;
}

hal_status_t hal_uart_write(hal_uart_t *uart, const uint8_t *buffer, size_t size,
                            size_t *transmitted_count)
{
    if (transmitted_count == NULL) {
        return HAL_STATUS_INVALID_ARGUMENT;
    }
    *transmitted_count = 0;
    if (uart == NULL || (buffer == NULL && size != 0)) {
        return HAL_STATUS_INVALID_ARGUMENT;
    }
    hal_uart_t object;
    memcpy(&object, uart, sizeof(object));
    if (!object.configured) {
        return HAL_STATUS_NOT_INITIALIZED;
    }
    if (object.data_width == HAL_UART_DATA_WIDTH_7) {
        for (size_t index = 0; index < size; ++index) {
            if (buffer[index] > 0x7f) {
                return HAL_STATUS_INVALID_ARGUMENT;
            }
        }
    }
    if (size == 0) {
        return HAL_STATUS_OK;
    }
    const int64_t deadline = esp_timer_get_time() + (int64_t)object.timeout * 1000;
    while (*transmitted_count < size) {
        if (esp_timer_get_time() >= deadline) {
            return HAL_STATUS_TIMEOUT;
        }
        size_t chunk = size - *transmitted_count;
        if (chunk > SOC_UART_FIFO_LEN) {
            chunk = SOC_UART_FIFO_LEN;
        }
        int accepted = uart_tx_chars(object.controller,
                                     (const char *)buffer + *transmitted_count, (uint32_t)chunk);
        if (accepted < 0 || (size_t)accepted > chunk) {
            return HAL_STATUS_ERROR;
        }
        *transmitted_count += (size_t)accepted;
        const int64_t remaining = deadline - esp_timer_get_time();
        if (remaining <= 0) {
            return HAL_STATUS_TIMEOUT;
        }
        const esp_err_t error = uart_wait_tx_done(object.controller, remaining_ticks(remaining));
        if (error != ESP_OK) {
            return hal_esp32_status(error);
        }
    }
    return esp_timer_get_time() > deadline ? HAL_STATUS_TIMEOUT : HAL_STATUS_OK;
}
