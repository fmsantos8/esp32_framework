#ifndef BSP_ESP32_H
#define BSP_ESP32_H

#include <stdbool.h>
#include <stdint.h>

#include "gpio/hal_gpio.h"
#include "i2c/hal_i2c.h"
#include "status/hal_status.h"
#include "timer/hal_timer.h"
#include "uart/hal_uart.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Configure the ESP32 DevKit V1 resources described in docs/pinout.md.
 *
 */
void bsp_esp32_init(void);

/**
 * @brief Get the button GPIO handle.
 *
 * @return hal_gpio_t* Handle to the button GPIO.
 */
hal_gpio_t *bsp_esp32_get_button(void);

/**
 * @brief Get the LED GPIO handle.
 *
 * @return hal_gpio_t* Handle to the LED GPIO.
 */
hal_gpio_t *bsp_esp32_get_led(void);

/**
 * @brief Get the I2C handle.
 *
 * @return hal_i2c_t* Handle to the I2C controller.
 */
hal_i2c_t *bsp_esp32_get_i2c(void);

/**
 * @brief Get the UART handle.
 *
 * @return hal_uart_t* Handle to the UART controller.
 */
hal_uart_t *bsp_esp32_get_uart(void);

/**
 * @brief Get the timer handle.
 *
 * @return hal_timer_t* Handle to the timer.
 */
hal_timer_t *bsp_esp32_get_timer(void);

#ifdef __cplusplus
}
#endif

#endif
