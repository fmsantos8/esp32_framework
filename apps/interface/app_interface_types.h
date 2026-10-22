/**
 * @file app_interface_types.h
 * @author Fabricio Santos (fmsantos8@ucs.br)
 * @brief Display dependencies and incremental rendering state.
 */
#ifndef APP_INTERFACE_TYPES_H
#define APP_INTERFACE_TYPES_H

#include "app_types.h"

/** @brief Borrowed display dependencies and failure recovery interval. */
typedef struct {
    ssd1306_t *display;                     /** Persistent display context. */
    const ssd1306_config_t *display_config; /** Persistent opaque construction configuration. */
    app_get_ticks_t get_ticks;              /** Coordinator's millisecond time source. */
    uint32_t retry;                         /** Positive retry interval, at most INT32_MAX. */
} app_interface_config_t;

/** @brief Caller-owned state; a frame completes before a newer snapshot is drawn. */
typedef struct {
    app_interface_config_t config; /** Borrowed dependencies and retry interval. */
    app_screen_t screen;           /** Last rendered screen. */
    uint32_t acquisition_revision; /** Last rendered acquisition result. */
    uint64_t uptime_seconds;       /** Last rendered diagnostic uptime. */
    uint32_t retry_since;          /** Timestamp after a failed display operation. */
    bool fault;                    /** Last rendered fault marker. */
    bool ready;                    /** Controller initialization succeeded. */
    bool init_attempted;           /** Controller startup has been attempted. */
    bool pending;                  /** Dirty pages remain in the current frame. */
    bool rendered;                 /** A snapshot has been drawn. */
    bool waiting_retry;            /** Controller recovery is delayed. */
    bool initialized;              /** Application context is available. */
} app_interface_t;

#endif
