#include "hal_timer_esp32.h"

#include "esp_err.h"

#define TIMER_RESOLUTION_HZ 1000000U

hal_status_t hal_timer_start(hal_timer_t *timer) {
    if (timer == NULL) {
        return HAL_STATUS_INVALID_PARAM;
    }
    if (timer->started) {
        return HAL_STATUS_OK;
    }

    gptimer_config_t config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = TIMER_RESOLUTION_HZ,
    };
    gptimer_handle_t driver = NULL;
    esp_err_t err = gptimer_new_timer(&config, &driver);
    if (err != ESP_OK) {
        return err == ESP_ERR_INVALID_ARG ? HAL_STATUS_NOT_SUPPORTED
                                          : HAL_STATUS_ERROR;
    }

    uint32_t resolution;
    if (gptimer_get_resolution(driver, &resolution) != ESP_OK) {
        gptimer_del_timer(driver);
        return HAL_STATUS_ERROR;
    }
    if (resolution != TIMER_RESOLUTION_HZ) {
        gptimer_del_timer(driver);
        return HAL_STATUS_NOT_SUPPORTED;
    }
    if (gptimer_set_raw_count(driver, 0) != ESP_OK) {
        gptimer_del_timer(driver);
        return HAL_STATUS_ERROR;
    }
    if (gptimer_enable(driver) != ESP_OK) {
        gptimer_del_timer(driver);
        return HAL_STATUS_ERROR;
    }
    if (gptimer_start(driver) != ESP_OK) {
        gptimer_disable(driver);
        gptimer_del_timer(driver);
        return HAL_STATUS_ERROR;
    }

    timer->driver = driver;
    timer->started = true;
    return HAL_STATUS_OK;
}

hal_status_t hal_timer_get_millis(hal_timer_t *timer, uint32_t *millis) {
    if (timer == NULL || millis == NULL) {
        return HAL_STATUS_INVALID_PARAM;
    }
    if (!timer->started) {
        return HAL_STATUS_ERROR;
    }

    uint64_t count;
    if (gptimer_get_raw_count(timer->driver, &count) != ESP_OK) {
        return HAL_STATUS_ERROR;
    }
    *millis = (uint32_t)(count / (TIMER_RESOLUTION_HZ / 1000U));
    return HAL_STATUS_OK;
}
