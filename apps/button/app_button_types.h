/**
 * @file app_button_types.h
 * @author Fabricio Santos (fmsantos8@ucs.br)
 * @brief Button application state.
 */
#ifndef APP_BUTTON_TYPES_H
#define APP_BUTTON_TYPES_H

#include "app_types.h"

/** @brief One screen change per press of an already debounced component. */
typedef struct {
    button_t *button; /** Borrowed initialized button. */
    bool armed;       /** A press may change the screen; release rearms it. */
    bool initialized; /** Context is available. */
} app_button_t;

#endif
