/**
 * @file hal_i2c.c
 * @author Fabricio Santos (fmsantos8@ucs.br)
 * @brief Synchronous addressed transactions on one shared ESP-IDF bus.
 *
 */
#include "i2c/hal_i2c_esp32.h"
#include "port_resources.h"
#include "port_status.h"
#include "framework_memory.h"

#include <stdbool.h>
#include <string.h>
#include "freertos/FreeRTOS.h"

static bool assigned_controllers[I2C_NUM_MAX];

hal_status_t hal_i2c_config(const hal_i2c_config_t *config, hal_i2c_t **out_i2c)
{
    if (out_i2c == NULL) {
        return HAL_STATUS_INVALID_ARGUMENT;
    }
    *out_i2c = NULL;
    if (config == NULL || config->instance >= I2C_NUM_MAX || config->frequency == 0 ||
        config->timeout == 0 || config->timeout > HAL_TIMEOUT_MAXIMUM ||
        (config->clock_pin.port == config->data_pin.port &&
         config->clock_pin.number == config->data_pin.number)) {
        return HAL_STATUS_INVALID_ARGUMENT;
    }
    hal_status_t status = hal_esp32_validate_pin(config->clock_pin, true);
    if (status != HAL_STATUS_OK) {
        return status;
    }
    status = hal_esp32_validate_pin(config->data_pin, true);
    if (status != HAL_STATUS_OK) {
        return status;
    }
    /* Conservative range for the ESP32 hardware timing registers. */
    if (config->frequency < 10000 || config->frequency > 400000) {
        return HAL_STATUS_NOT_SUPPORTED;
    }
    /* The SDK converts milliseconds using scheduler ticks. Avoid zero ticks
     * and multiplication overflow in SDK versions using 32-bit conversion. */
    if (config->timeout < (1000u + configTICK_RATE_HZ - 1u) / configTICK_RATE_HZ ||
        config->timeout > UINT32_MAX / configTICK_RATE_HZ) {
        return HAL_STATUS_NOT_SUPPORTED;
    }
    status = hal_esp32_check_startup();
    if (status != HAL_STATUS_OK) {
        return status;
    }
    if (assigned_controllers[config->instance] || hal_esp32_pin_assigned(config->clock_pin) ||
        hal_esp32_pin_assigned(config->data_pin)) {
        return HAL_STATUS_RESOURCE_BUSY;
    }
    void *storage = NULL;
    status = framework_memory_allocate(sizeof(hal_i2c_t), _Alignof(hal_i2c_t), &storage);
    if (status != HAL_STATUS_OK) {
        return status;
    }
    const i2c_master_bus_config_t driver_config = {
        .i2c_port = config->instance,
        .sda_io_num = config->data_pin.number,
        .scl_io_num = config->clock_pin.number,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    hal_i2c_t object = {0};
    esp_err_t error = i2c_new_master_bus(&driver_config, &object.bus);
    if (error != ESP_OK) {
        return error == ESP_ERR_NOT_FOUND ? HAL_STATUS_RESOURCE_BUSY : hal_esp32_status(error);
    }
    const i2c_device_config_t device_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = I2C_DEVICE_ADDRESS_NOT_USED,
        .scl_speed_hz = config->frequency,
    };
    error = i2c_master_bus_add_device(object.bus, &device_config, &object.device);
    if (error != ESP_OK) {
        if (i2c_del_master_bus(object.bus) != ESP_OK) {
            assigned_controllers[config->instance] = true;
            hal_esp32_claim_pin(config->clock_pin);
            hal_esp32_claim_pin(config->data_pin);
        }
        return hal_esp32_status(error);
    }
    object.timeout = config->timeout;
    memcpy(storage, &object, sizeof(object));
    assigned_controllers[config->instance] = true;
    hal_esp32_claim_pin(config->clock_pin);
    hal_esp32_claim_pin(config->data_pin);
    *out_i2c = storage;
    return HAL_STATUS_OK;
}

/**
 * @brief Validate one operation without starting any bus activity.
 */
static hal_status_t validate_transfer(hal_i2c_t *i2c, uint8_t address,
                                      const void *buffer, size_t size)
{
    if (i2c == NULL || buffer == NULL || size == 0 || address < 0x08 || address > 0x77) {
        return HAL_STATUS_INVALID_ARGUMENT;
    }
    if (size > UINT16_MAX) {
        return HAL_STATUS_NOT_SUPPORTED;
    }
    return HAL_STATUS_OK;
}

/**
 * @brief Execute one complete command list and retain no caller buffer reference.
 */
static hal_status_t execute_transfer(hal_i2c_t *i2c, i2c_operation_job_t *operations,
                                     size_t count)
{
    hal_i2c_t object;
    memcpy(&object, i2c, sizeof(object));
    if (object.bus == NULL || object.device == NULL) {
        return HAL_STATUS_NOT_INITIALIZED;
    }
    const esp_err_t error = i2c_master_execute_defined_operations(object.device, operations,
                                                                 count, (int)object.timeout);
    return hal_esp32_status(error);
}

hal_status_t hal_i2c_write(hal_i2c_t *i2c, uint8_t address, const uint8_t *buffer,
                           size_t size)
{
    hal_status_t status = validate_transfer(i2c, address, buffer, size);
    if (status != HAL_STATUS_OK) {
        return status;
    }
    const uint8_t address_byte = (uint8_t)(address << 1);
    i2c_operation_job_t operations[] = {
        {.command = I2C_MASTER_CMD_START},
        {.command = I2C_MASTER_CMD_WRITE,
         .write = {.ack_check = true, .data = &address_byte, .total_bytes = 1}},
        {.command = I2C_MASTER_CMD_WRITE,
         .write = {.ack_check = true, .data = buffer, .total_bytes = size}},
        {.command = I2C_MASTER_CMD_STOP},
    };
    return execute_transfer(i2c, operations, sizeof(operations) / sizeof(operations[0]));
}

hal_status_t hal_i2c_read(hal_i2c_t *i2c, uint8_t address, uint8_t *buffer,
                          size_t size)
{
    hal_status_t status = validate_transfer(i2c, address, buffer, size);
    if (status != HAL_STATUS_OK) {
        return status;
    }
    const uint8_t address_byte = (uint8_t)((address << 1) | 1u);
    i2c_operation_job_t operations[5] = {
        {.command = I2C_MASTER_CMD_START},
        {.command = I2C_MASTER_CMD_WRITE,
         .write = {.ack_check = true, .data = &address_byte, .total_bytes = 1}},
    };
    size_t count = 2;
    if (size > 1) {
        operations[count++] = (i2c_operation_job_t){.command = I2C_MASTER_CMD_READ,
            .read = {.ack_value = I2C_ACK_VAL, .data = buffer, .total_bytes = size - 1}};
    }
    operations[count++] = (i2c_operation_job_t){.command = I2C_MASTER_CMD_READ,
        .read = {.ack_value = I2C_NACK_VAL, .data = buffer + size - 1, .total_bytes = 1}};
    operations[count++] = (i2c_operation_job_t){.command = I2C_MASTER_CMD_STOP};
    return execute_transfer(i2c, operations, count);
}

hal_status_t hal_i2c_write_read(hal_i2c_t *i2c, uint8_t address,
                                const uint8_t *write_buffer, size_t write_size,
                                uint8_t *read_buffer, size_t read_size)
{
    hal_status_t status = validate_transfer(i2c, address, write_buffer, write_size);
    if (status != HAL_STATUS_OK) {
        return status;
    }
    status = validate_transfer(i2c, address, read_buffer, read_size);
    if (status != HAL_STATUS_OK) {
        return status;
    }
    const uintptr_t write_start = (uintptr_t)write_buffer;
    const uintptr_t read_start = (uintptr_t)read_buffer;
    if (write_start <= read_start ? read_start - write_start < write_size :
                                   write_start - read_start < read_size) {
        return HAL_STATUS_INVALID_ARGUMENT;
    }
    const uint8_t write_address = (uint8_t)(address << 1);
    const uint8_t read_address = (uint8_t)(write_address | 1u);
    i2c_operation_job_t operations[8] = {
        {.command = I2C_MASTER_CMD_START},
        {.command = I2C_MASTER_CMD_WRITE,
         .write = {.ack_check = true, .data = &write_address, .total_bytes = 1}},
        {.command = I2C_MASTER_CMD_WRITE,
         .write = {.ack_check = true, .data = write_buffer, .total_bytes = write_size}},
        {.command = I2C_MASTER_CMD_START},
        {.command = I2C_MASTER_CMD_WRITE,
         .write = {.ack_check = true, .data = &read_address, .total_bytes = 1}},
    };
    size_t count = 5;
    if (read_size > 1) {
        operations[count++] = (i2c_operation_job_t){.command = I2C_MASTER_CMD_READ,
            .read = {.ack_value = I2C_ACK_VAL, .data = read_buffer, .total_bytes = read_size - 1}};
    }
    operations[count++] = (i2c_operation_job_t){.command = I2C_MASTER_CMD_READ,
        .read = {.ack_value = I2C_NACK_VAL, .data = read_buffer + read_size - 1, .total_bytes = 1}};
    operations[count++] = (i2c_operation_job_t){.command = I2C_MASTER_CMD_STOP};
    return execute_transfer(i2c, operations, count);
}
