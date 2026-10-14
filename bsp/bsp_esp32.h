/**
 * @file bsp_esp32.h
 * @author Fabricio Santos (fmsantos8@ucs.br)
 * @brief ESP32 DevKit V1 resource construction and board wiring.
 *
 */
#ifndef BSP_ESP32_H
#define BSP_ESP32_H

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
 * @brief Construct the resources described in docs/pinout.md during startup.
 *
 * @details Requires an initialized, unsealed arena. The LED is configured off
 * first. Failed startup is returned to integration without automatic retry.
 *
 * @return Common construction status; HAL_STATUS_INVALID_STATE on repeated calls.
 */
hal_status_t bsp_esp32_init(void);

/**
 * @brief Borrow the button input, or NULL before successful construction.
 */
hal_gpio_t *bsp_esp32_get_button(void);
/**
 * @brief Borrow the active-high LED output, or NULL before construction.
 */
hal_gpio_t *bsp_esp32_get_led(void);
/**
 * @brief Borrow the bus shared by the sensor and display, or NULL before construction.
 */
hal_i2c_t *bsp_esp32_get_i2c(void);
/**
 * @brief Borrow UART0 for 115200-baud 8N1 telemetry, or NULL before construction.
 */
hal_uart_t *bsp_esp32_get_uart(void);
/**
 * @brief Borrow the elapsed-time source, or NULL before construction.
 */
hal_timer_t *bsp_esp32_get_timer(void);
/**
 * @brief Return the electrical level representing a pressed button.
 */
bool bsp_esp32_get_button_active_level(void);
/**
 * @brief Return the electrical level representing an illuminated LED.
 */
bool bsp_esp32_get_led_active_level(void);
/**
 * @brief Return the SHT31-D unshifted seven-bit I2C address.
 */
uint8_t bsp_esp32_get_sht31_address(void);
/**
 * @brief Return the SSD1306 unshifted seven-bit I2C address.
 */
uint8_t bsp_esp32_get_ssd1306_address(void);

#ifdef __cplusplus
}
#endif

#endif
