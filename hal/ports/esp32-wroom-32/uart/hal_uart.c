#include "hal_uart_esp32.h"
#include "port_status.h"

#include <limits.h>

#include "freertos/FreeRTOS.h"

#define UART_RX_BUFFER_SIZE   256
#define UART_TX_FIFO_CAPACITY 128U

static bool is_valid_config(const hal_uart_config_t *config) {
    return config != NULL && config->baud_rate != 0 &&
           (config->parity == HAL_UART_PARITY_NONE ||
            config->parity == HAL_UART_PARITY_EVEN ||
            config->parity == HAL_UART_PARITY_ODD) &&
           (config->stop_bits == HAL_UART_STOP_BITS_1 ||
            config->stop_bits == HAL_UART_STOP_BITS_2);
}

static bool is_supported_uart(const hal_uart_t *uart) {
    return uart->port >= 0 && uart->port < UART_NUM_MAX &&
           GPIO_IS_VALID_OUTPUT_GPIO(uart->tx_pin) &&
           GPIO_IS_VALID_GPIO(uart->rx_pin) &&
           uart->tx_pin != uart->rx_pin;
}

static uart_parity_t get_parity_mode(hal_uart_parity_t parity) {
    switch (parity) {
        case HAL_UART_PARITY_NONE:
            return UART_PARITY_DISABLE;
        case HAL_UART_PARITY_EVEN:
            return UART_PARITY_EVEN;
        case HAL_UART_PARITY_ODD:
            return UART_PARITY_ODD;
        default:
            return UART_PARITY_DISABLE;
    }
}

static uart_stop_bits_t get_stop_bits_mode(hal_uart_stop_bits_t stop_bits) {
    switch (stop_bits) {
        case HAL_UART_STOP_BITS_1:
            return UART_STOP_BITS_1;
        case HAL_UART_STOP_BITS_2:
            return UART_STOP_BITS_2;
        default:
            return UART_STOP_BITS_1;
    }
}

hal_status_t hal_uart_config(hal_uart_t *uart, const hal_uart_config_t *config) {
    if (uart == NULL || !is_valid_config(config)) {
        return HAL_STATUS_INVALID_PARAM;
    }
    if (config->baud_rate > INT_MAX) {
        return HAL_STATUS_NOT_SUPPORTED;
    }
    if (!is_supported_uart(uart)) {
        return HAL_STATUS_NOT_SUPPORTED;
    }

    uart_config_t driver_config = {
        .baud_rate = (int)config->baud_rate,
        .data_bits = UART_DATA_8_BITS,
        .parity = get_parity_mode(config->parity),
        .stop_bits = get_stop_bits_mode(config->stop_bits),
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    bool installed_here = false;
    if (!uart->configured) {
        if (uart_driver_install(uart->port, UART_RX_BUFFER_SIZE, 0, 0, NULL, 0) != ESP_OK) {
            return HAL_STATUS_ERROR;
        }
        installed_here = true;
    }
    esp_err_t err = uart_param_config(uart->port, &driver_config);
    if (err != ESP_OK) {
        if (installed_here) {
            uart_driver_delete(uart->port);
        }
        return err == ESP_FAIL || err == ESP_ERR_NOT_SUPPORTED
                   ? HAL_STATUS_NOT_SUPPORTED
                   : HAL_STATUS_ERROR;
    }
    err = uart_set_pin(uart->port, uart->tx_pin, uart->rx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (err != ESP_OK) {
        if (installed_here) {
            uart_driver_delete(uart->port);
        }
        return hal_esp32_status(err);
    }
    uart->baud_rate = config->baud_rate;
    uart->configured = true;
    return HAL_STATUS_OK;
}

hal_status_t hal_uart_write(hal_uart_t *uart, const uint8_t *data, uint32_t size) {
    if (uart == NULL || data == NULL || size == 0) {
        return HAL_STATUS_INVALID_PARAM;
    }
    if (!uart->configured) {
        return HAL_STATUS_ERROR;
    }

    uint32_t offset = 0;
    while (offset < size) {
        size_t chunk = size - offset;
        if (chunk > INT_MAX) {
            chunk = INT_MAX;
        }
        int written = uart_write_bytes(uart->port, data + offset, chunk);
        if (written <= 0) {
            return HAL_STATUS_ERROR;
        }
        offset += (uint32_t)written;
    }

    /* With no TX ring buffer, at most the hardware FIFO remains in flight. */
    uint32_t timeout_ms = 1000U +
                          (uint32_t)((UART_TX_FIFO_CAPACITY * 12000ULL + uart->baud_rate - 1) /
                                     uart->baud_rate);
    return hal_esp32_status(uart_wait_tx_done(uart->port, pdMS_TO_TICKS(timeout_ms)));
}
