/**
 * @file app_gui.h
 * @author Fabricio Santos (fmsantos8@ucs.br)
 * @brief Presents sensor readings and acquisition diagnostics on the OLED.
 */
#ifndef APP_GUI_H
#define APP_GUI_H

#include "cmp_ssd1306.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the GUI on the readings screen
 *
 * @param[in] get_ticks_function Required function to read the current time (ms)
 *
 * @retval true Task initialized successfully.
 * @retval false get_ticks_function is NULL; previous state is preserved.
 */
bool app_gui_init(uint32_t (*get_ticks_function)(void));

/**
 * @brief Update the display with the latest readings or diagnostics
 *
 * @param[in,out] display Display component
 */
void app_gui_run(ssd1306_t *display);

/**
 * @brief Alternate between readings and diagnostics after initialization.
 */
void app_gui_toggle_screen(void);

#ifdef __cplusplus
}
#endif

#endif
