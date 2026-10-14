/**
 * @file port_resources.h
 * @author Fabricio Santos (fmsantos8@ucs.br)
 * @brief Private startup checks and shared ESP32 pin ownership.
 *
 */
#ifndef ESP32_PORT_RESOURCES_H
#define ESP32_PORT_RESOURCES_H

#include <stdbool.h>
#include "common/hal_pin.h"
#include "common/hal_status.h"

/**
 * @brief Check the arena lifecycle without allocating.
 */
hal_status_t hal_esp32_check_startup(void);

/**
 * @brief Validate bank 0, pin capabilities, and WROOM-32 flash reservations.
 */
hal_status_t hal_esp32_validate_pin(hal_pin_t pin, bool output);

/**
 * @brief Report whether a pin is assigned to a framework resource.
 */
bool hal_esp32_pin_assigned(hal_pin_t pin);

/**
 * @brief Claim a validated pin after successful peripheral construction.
 */
void hal_esp32_claim_pin(hal_pin_t pin);

#endif
