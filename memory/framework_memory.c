/**
 * @file framework_memory.c
 * @author Fabricio Santos (fmsantos8@ucs.br)
 * @brief Monotonic startup allocation from aligned static byte storage.
 *
 */
#include "framework_memory.h"

#include <stdint.h>

#ifndef FRAMEWORK_MEMORY_CAPACITY
#error "Define FRAMEWORK_MEMORY_CAPACITY when building the memory component."
#endif

_Static_assert(FRAMEWORK_MEMORY_CAPACITY > 0, "The arena must not be empty.");

_Alignas(max_align_t) static unsigned char storage[FRAMEWORK_MEMORY_CAPACITY];
static size_t allocated;
static bool initialized;
static bool sealed;

hal_status_t framework_memory_init(void)
{
    if (initialized) {
        return HAL_STATUS_INVALID_STATE;
    }
    initialized = true;
    return HAL_STATUS_OK;
}

hal_status_t framework_memory_allocate(size_t size, size_t alignment, void **memory)
{
    if (memory == NULL) {
        return HAL_STATUS_INVALID_ARGUMENT;
    }
    *memory = NULL;
    if (size == 0 || alignment == 0 || (alignment & (alignment - 1)) != 0) {
        return HAL_STATUS_INVALID_ARGUMENT;
    }
    if (!initialized) {
        return HAL_STATUS_NOT_INITIALIZED;
    }
    if (sealed) {
        return HAL_STATUS_INVALID_STATE;
    }
    if (alignment > _Alignof(max_align_t)) {
        return HAL_STATUS_NOT_SUPPORTED;
    }
    const size_t padding = ((size_t)0 - allocated) & (alignment - 1);
    const size_t remaining = sizeof(storage) - allocated;
    if (padding > remaining || size > remaining - padding) {
        return HAL_STATUS_NO_MEMORY;
    }
    *memory = storage + allocated + padding;
    allocated += padding + size;
    return HAL_STATUS_OK;
}

hal_status_t framework_memory_seal(void)
{
    if (!initialized) {
        return HAL_STATUS_NOT_INITIALIZED;
    }
    sealed = true;
    return HAL_STATUS_OK;
}

hal_status_t framework_memory_get_usage(framework_memory_usage_t *usage)
{
    if (usage == NULL) {
        return HAL_STATUS_INVALID_ARGUMENT;
    }
    if (!initialized) {
        return HAL_STATUS_NOT_INITIALIZED;
    }
    const framework_memory_usage_t snapshot = {
        .capacity = sizeof(storage),
        .allocated = allocated,
        .remaining = sizeof(storage) - allocated,
        .sealed = sealed,
    };
    *usage = snapshot;
    return HAL_STATUS_OK;
}
