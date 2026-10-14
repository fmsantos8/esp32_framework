/**
 * @file framework_memory.h
 * @author Fabricio Santos (fmsantos8@ucs.br)
 * @brief Startup-only allocation from a statically reserved memory arena.
 *
 */
#ifndef FRAMEWORK_MEMORY_H
#define FRAMEWORK_MEMORY_H

#include <stdbool.h>
#include <stddef.h>

#include "common/hal_status.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Version of the monotonic memory contract.
 *
 * @details Integration configures the arena capacity at build time. Ports request storage
 * for their opaque objects; components do not allocate or release those objects.
 * Startup calls are sequential. There is no heap, per-object deallocation, or
 * public reset. Returned addresses remain stable for the firmware lifetime.
 *
 * The allocator advances through the arena and includes alignment padding in
 * consumed space. Failed allocation leaves the cursor unchanged. An allocation
 * followed by failed peripheral construction remains consumed unless an internal,
 * unpublished reservation mechanism is provided by the implementation.
 *
 * The backing storage must satisfy both alignment and the C language's typed
 * access rules. Aligning a uint8_t array alone does not establish that arbitrary
 * SDK structures may be accessed through cast pointers. Ports and integration
 * must select a supported storage strategy; this header prescribes no unsafe cast.
 */
#define FRAMEWORK_MEMORY_CONTRACT_VERSION 1u

/**
 * @brief Snapshot of the static arena's current usage.
 */
typedef struct {
    size_t capacity;  /** Total reserved storage in bytes */
    size_t allocated; /** Consumed bytes, including alignment padding */
    size_t remaining; /** Tail bytes available; capacity minus allocated */
    bool sealed;      /** True when further allocation is forbidden */
} framework_memory_usage_t;

/**
 * @brief Open the statically configured arena once during startup.
 *
 * @details Repeated initialization must not clear storage or invalidate handles.
 * Peripheral constructors require this call to have succeeded.
 *
 * @retval HAL_STATUS_OK The arena is open with no consumed storage.
 * @retval HAL_STATUS_INVALID_STATE The arena was already initialized, including if sealed.
 * @retval HAL_STATUS_ERROR The build-time storage configuration cannot be used.
 */
hal_status_t framework_memory_init(void);

/**
 * @brief Reserve suitably aligned storage without using the heap.
 *
 * @details The implementation validates arithmetic before changing the cursor,
 * including alignment padding and size overflow. The returned storage is not
 * promised to contain an initialized typed object; its constructor performs
 * initialization using a storage strategy valid for the selected toolchain.
 *
 * @param[in] size Requested object storage in bytes; must be positive.
 * @param[in] alignment Required byte alignment; must be a nonzero power of two.
 * @param[out] memory Receives stable storage, or NULL on failure.
 *
 * @retval HAL_STATUS_OK The entire request was reserved with the requested alignment.
 * @retval HAL_STATUS_INVALID_ARGUMENT The output pointer, size, or alignment is invalid.
 * @retval HAL_STATUS_NOT_SUPPORTED The arena cannot provide the requested alignment.
 * @retval HAL_STATUS_NOT_INITIALIZED The arena has not been initialized.
 * @retval HAL_STATUS_INVALID_STATE The arena is sealed.
 * @retval HAL_STATUS_NO_MEMORY The request, including padding, exceeds remaining storage.
 */
hal_status_t framework_memory_allocate(size_t size,
                                       size_t alignment,
                                       void **memory);

/**
 * @brief End startup allocation without invalidating existing objects.
 *
 * @details Sealing is idempotent. Peripheral reads and writes remain available,
 * but subsequent allocation and constructor calls return HAL_STATUS_INVALID_STATE.
 *
 * @retval HAL_STATUS_OK The initialized arena is sealed, including if already sealed.
 * @retval HAL_STATUS_NOT_INITIALIZED The arena has not been initialized.
 */
hal_status_t framework_memory_seal(void);

/**
 * @brief Read arena usage without allocating or modifying storage.
 *
 * @param[out] usage Receives the snapshot only on success.
 *
 * @retval HAL_STATUS_OK The initialized arena's usage was obtained.
 * @retval HAL_STATUS_INVALID_ARGUMENT The output pointer is NULL.
 * @retval HAL_STATUS_NOT_INITIALIZED The arena has not been initialized.
 */
hal_status_t framework_memory_get_usage(framework_memory_usage_t *usage);

#ifdef __cplusplus
}
#endif

#endif /* FRAMEWORK_MEMORY_H */
