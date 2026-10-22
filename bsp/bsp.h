/**
 * @file bsp.h
 * @author Fabricio Santos (fmsantos8@ucs.br)
 * @brief Hardware resources provided by the selected board.
 */
#ifndef BSP_H
#define BSP_H

#include <stdbool.h>
#include <stdint.h>

#include "common/hal_status.h"
#include "gpio/hal_gpio.h"
#include "i2c/hal_i2c.h"
#include "timer/hal_timer.h"
#include "uart/hal_uart.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Constructed HAL resources and electrical characteristics of the board.
 *
 * @details Handles refer to persistent objects stored in the HAL arena. The
 * composition supplies these resources to components without transferring
 * ownership. The sensor and display share devices_i2c and use distinct addresses.
 */
typedef struct {
    hal_gpio_t *button;          /** Button input GPIO. */
    hal_gpio_t *led;             /** Status LED output GPIO. */
    hal_i2c_t *devices_i2c;      /** Bus shared by the sensor and display. */
    hal_uart_t *telemetry_uart;  /** UART used for telemetry. */
    hal_timer_t *timebase_timer; /** Stopped timer configured with a 1 ms period. */

    bool button_active_level; /** Electrical level representing a pressed button. */
    bool led_active_level;    /** Electrical level that illuminates the LED. */

    uint8_t sht31_address;   /** Unshifted seven-bit SHT31-D address. */
    uint8_t ssd1306_address; /** Unshifted seven-bit SSD1306 address. */
} bsp_resources_t;

/**
 * @brief Construct the selected board's hardware resources once.
 *
 * @details Call sequentially from normal execution context after
 * hal_system_init() and before hal_system_seal(). The LED is configured off
 * first, and the timer remains stopped. The composition must initialize all
 * state shared with timer_callback before starting the timer.
 *
 * Invalid arguments are rejected without beginning construction. Once
 * construction begins, further attempts are rejected, including after failure.
 * A failed construction may consume arena space and reserve hardware resources;
 * restart the firmware before retrying. out_resources is modified only on success
 * and does not need to be initialized before this call.
 *
 * @param[in] timer_callback Required callback invoked in interrupt context for
 * each serviced timer expiration, with a nominal period of one millisecond.
 * @param[out] out_resources Receives all constructed resources on success.
 *
 * @retval HAL_STATUS_OK All resources were constructed.
 * @retval HAL_STATUS_INVALID_ARGUMENT timer_callback or out_resources is NULL.
 * @retval HAL_STATUS_INVALID_STATE Construction has already been attempted, or
 * HAL resource construction is closed.
 * @retval HAL_STATUS_NOT_INITIALIZED The system has not been initialized.
 * @retval HAL_STATUS_NOT_SUPPORTED A board configuration is unavailable.
 * @retval HAL_STATUS_RESOURCE_BUSY A required hardware resource is already claimed.
 * @retval HAL_STATUS_NO_MEMORY HAL or SDK object storage is unavailable.
 * @retval HAL_STATUS_ERROR A peripheral could not be configured.
 */
hal_status_t bsp_init(hal_timer_callback_t timer_callback, bsp_resources_t *out_resources);

#ifdef __cplusplus
}
#endif

#endif
