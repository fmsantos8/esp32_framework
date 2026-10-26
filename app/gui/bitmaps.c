/**
 * @file bitmaps.c
 * @author Fabricio Santos (fmsantos8@ucs.br)
 * @brief Temperature and humidity bitmap data for the GUI.
 */
#include "bitmaps.h"

/** @brief Nine-pixel thermometer, one two-byte row per scanline. */
static const uint8_t thermometer_data[][2] = {
    {0x1C, 0x00},
    {0x22, 0x00},
    {0x22, 0x00},
    {0x22, 0x00},
    {0x22, 0x00},
    {0x2A, 0x00},
    {0x2A, 0x00},
    {0x2A, 0x00},
    {0x2A, 0x00},
    {0x2A, 0x00},
    {0x3E, 0x00},
    {0x6B, 0x00},
    {0x5D, 0x00},
    {0xBE, 0x80},
    {0xBE, 0x80},
    {0xBE, 0x80},
    {0x5D, 0x00},
    {0x63, 0x00},
    {0x1C, 0x00},
};

/** @brief Nine-pixel humidity droplet, one two-byte row per scanline. */
static const uint8_t droplet_data[][2] = {
    {0x08, 0x00},
    {0x14, 0x00},
    {0x14, 0x00},
    {0x22, 0x00},
    {0x22, 0x00},
    {0x41, 0x00},
    {0x41, 0x00},
    {0x80, 0x80},
    {0xA0, 0x80},
    {0xA0, 0x80},
    {0xA0, 0x80},
    {0x41, 0x00},
    {0x3E, 0x00},
};

/** @brief Temperature icon backed by immutable pixel data. */
static const ssd1306_bitmap_t thermometer_bitmap = {
    .data = (const uint8_t *)&thermometer_data,
    .size = sizeof(thermometer_data),
    .width = 9,
    .height = 19,
};

/** @brief Humidity icon backed by immutable pixel data. */
static const ssd1306_bitmap_t droplet_bitmap = {
    .data = (const uint8_t *)&droplet_data,
    .size = sizeof(droplet_data),
    .width = 9,
    .height = 13,
};

const ssd1306_bitmap_t *get_thermometer_bitmap(void) {
    return &thermometer_bitmap;
}

const ssd1306_bitmap_t *get_droplet_bitmap(void) {
    return &droplet_bitmap;
}
