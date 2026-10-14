/**
 * @file hal_timer.c
 * @author Fabricio Santos (fmsantos8@ucs.br)
 * @brief Elapsed milliseconds from the continuously running ESP-IDF clock.
 *
 */
#include "timer/hal_timer_esp32.h"
#include "port_resources.h"
#include "framework_memory.h"
#include "esp_timer.h"

#include <string.h>

static bool assigned;

hal_status_t hal_timer_config(const hal_timer_config_t *config, hal_timer_t **out_timer)
{
    if (out_timer == NULL) {
        return HAL_STATUS_INVALID_ARGUMENT;
    }
    *out_timer = NULL;
    if (config == NULL || config->instance != 0) {
        return HAL_STATUS_INVALID_ARGUMENT;
    }
    hal_status_t status = hal_esp32_check_startup();
    if (status != HAL_STATUS_OK) {
        return status;
    }
    if (assigned) {
        return HAL_STATUS_RESOURCE_BUSY;
    }
    const int64_t origin = esp_timer_get_time();
    if (origin < 0) {
        return HAL_STATUS_ERROR;
    }
    void *storage = NULL;
    status = framework_memory_allocate(sizeof(hal_timer_t), _Alignof(hal_timer_t), &storage);
    if (status != HAL_STATUS_OK) {
        return status;
    }
    const hal_timer_t object = {.origin = origin, .configured = true};
    memcpy(storage, &object, sizeof(object));
    assigned = true;
    *out_timer = storage;
    return HAL_STATUS_OK;
}

hal_status_t hal_timer_read(hal_timer_t *timer, uint32_t *elapsed_time)
{
    if (timer == NULL || elapsed_time == NULL) {
        return HAL_STATUS_INVALID_ARGUMENT;
    }
    hal_timer_t object;
    memcpy(&object, timer, sizeof(object));
    if (!object.configured) {
        return HAL_STATUS_NOT_INITIALIZED;
    }
    const int64_t current_time = esp_timer_get_time();
    if (current_time < object.origin) {
        return HAL_STATUS_ERROR;
    }
    *elapsed_time = (uint32_t)((uint64_t)(current_time - object.origin) / 1000);
    return HAL_STATUS_OK;
}
