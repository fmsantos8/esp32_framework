/**
 * @file app_acquisition_types.h
 * @author Fabricio Santos (fmsantos8@ucs.br)
 * @brief Acquisition dependencies and scheduling state.
 */
#ifndef APP_ACQUISITION_TYPES_H
#define APP_ACQUISITION_TYPES_H

#include "app_types.h"

/** @brief Borrowed sensor dependencies and positive millisecond intervals. */
typedef struct {
    sht31d_t *sensor;                     /** Persistent sensor context. */
    const sht31d_config_t *sensor_config; /** Persistent opaque construction configuration. */
    app_get_ticks_t get_ticks;            /** Same time source used by the coordinator. */
    uint32_t period;                      /** Start-to-start acquisition interval, at least 16 ms. */
    uint32_t retry;                       /** Delay after failure, at least 16 ms. */
} app_acquisition_config_t;

/** @brief Caller-owned acquisition state; initialize before processing. */
typedef struct {
    app_acquisition_config_t config; /** Borrowed dependencies and scheduling parameters. */
    uint32_t since;                  /** Beginning of the current wait. */
    uint32_t delay;                  /** Required wait in milliseconds. */
    uint32_t cycle_started;          /** Timestamp after the last successful start returned. */
    bool initialized;                /** Application context is available. */
    bool init_attempted;             /** Sensor initialization has been attempted. */
    bool ready;                      /** Sensor initialization succeeded. */
    bool converting;                 /** A single-shot conversion is pending. */
} app_acquisition_t;

#endif
