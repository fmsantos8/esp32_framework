/**
 * @file bitmaps.h
 * @author Fabricio Santos (fmsantos8@ucs.br)
 * @brief Provides access to the GUI icon bitmaps.
 */
#ifndef BITMAPS_H
#define BITMAPS_H

#include "cmp_ssd1306.h"

/**
 * @brief Get the thermometer bitmap.
 *
 * @retval Thermometer bitmap
 */
const ssd1306_bitmap_t *get_thermometer_bitmap(void);

/**
 * @brief Get the humidity droplet bitmap.
 *
 * @retval Droplet bitmap
 */
const ssd1306_bitmap_t *get_droplet_bitmap(void);

#endif
