#include "hal_i2c_esp32.h"
#include "port_status.h"

#include <stdbool.h>

#define I2C_TRANSFER_TIMEOUT_MS 1000
#define I2C_MAX_CLOCK_HZ        400000U

static bool is_valid_config(const hal_i2c_config_t *config) {
    return config != NULL && config->clock_speed != 0;
}

static bool is_supported_bus(const hal_i2c_t *i2c) {
    return i2c->controller >= 0 && i2c->controller < I2C_NUM_MAX &&
           GPIO_IS_VALID_OUTPUT_GPIO(i2c->sda_pin) &&
           GPIO_IS_VALID_OUTPUT_GPIO(i2c->scl_pin) &&
           i2c->sda_pin != i2c->scl_pin;
}

hal_status_t hal_i2c_config(hal_i2c_t *i2c, const hal_i2c_config_t *config) {
    if (i2c == NULL || !is_valid_config(config)) {
        return HAL_STATUS_INVALID_PARAM;
    }
    if (config->clock_speed > I2C_MAX_CLOCK_HZ) {
        return HAL_STATUS_NOT_SUPPORTED;
    }
    if (!is_supported_bus(i2c)) {
        return HAL_STATUS_NOT_SUPPORTED;
    }
    if (i2c->bus == NULL) {
        i2c_master_bus_config_t bus_config = {
            .i2c_port = i2c->controller,
            .sda_io_num = i2c->sda_pin,
            .scl_io_num = i2c->scl_pin,
            .clk_source = I2C_CLK_SRC_DEFAULT,
            .glitch_ignore_cnt = 7,
            .flags.enable_internal_pullup = true,
        };
        esp_err_t err = i2c_new_master_bus(&bus_config, &i2c->bus);
        if (err != ESP_OK) {
            return hal_esp32_status(err);
        }
    }
    i2c->clock_speed = config->clock_speed;
    return HAL_STATUS_OK;
}

hal_status_t hal_i2c_write(hal_i2c_t *i2c, uint8_t address, const uint8_t *data, uint32_t size) {
    if (i2c == NULL || data == NULL || size == 0 || address > 0x7f) {
        return HAL_STATUS_INVALID_PARAM;
    }
    if (i2c->bus == NULL || i2c->clock_speed == 0) {
        return HAL_STATUS_ERROR;
    }

    i2c_device_config_t device_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = address,
        .scl_speed_hz = i2c->clock_speed,
    };
    i2c_master_dev_handle_t device = NULL;
    esp_err_t err = i2c_master_bus_add_device(i2c->bus, &device_config, &device);
    if (err != ESP_OK) {
        return hal_esp32_status(err);
    }

    err = i2c_master_transmit(device, data, size, I2C_TRANSFER_TIMEOUT_MS);
    esp_err_t remove_err = i2c_master_bus_rm_device(device);
    if (err != ESP_OK) {
        return hal_esp32_status(err);
    }
    return hal_esp32_status(remove_err);
}

hal_status_t hal_i2c_read(hal_i2c_t *i2c, uint8_t address, uint8_t *data, uint32_t size) {
    if (i2c == NULL || data == NULL || size == 0 || address > 0x7f) {
        return HAL_STATUS_INVALID_PARAM;
    }
    if (i2c->bus == NULL || i2c->clock_speed == 0) {
        return HAL_STATUS_ERROR;
    }

    i2c_device_config_t device_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = address,
        .scl_speed_hz = i2c->clock_speed,
    };
    i2c_master_dev_handle_t device = NULL;
    esp_err_t err = i2c_master_bus_add_device(i2c->bus, &device_config, &device);
    if (err != ESP_OK) {
        return hal_esp32_status(err);
    }

    err = i2c_master_receive(device, data, size, I2C_TRANSFER_TIMEOUT_MS);
    esp_err_t remove_err = i2c_master_bus_rm_device(device);
    if (err != ESP_OK) {
        return hal_esp32_status(err);
    }
    return hal_esp32_status(remove_err);
}
