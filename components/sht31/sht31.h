/**
 * @file sht31.h
 * @brief Single-shot SHT31-D acquisition, independent of HAL and SDK types.
 */
#ifndef SHT31_H
#define SHT31_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Caller-owned context; its storage definition is in sht31_config.h. */
typedef struct sht31 sht31_t;

/** @brief Construction dependencies, defined in sht31_config.h. */
typedef struct sht31_config sht31_config_t;

/** @brief Component results, independent of HAL and SDK status types. */
typedef enum {
    SHT31_STATUS_OK = 0,              /** Operation completed successfully. */
    SHT31_STATUS_INVALID_ARGUMENT,    /** A required pointer or device address is invalid. */
    SHT31_STATUS_NOT_INITIALIZED,     /** The sensor or its I2C dependency is not initialized. */
    SHT31_STATUS_INVALID_STATE,       /** A measurement is already pending, or none is pending for reading. */
    SHT31_STATUS_NOT_SUPPORTED,       /** A required HAL operation is unavailable. */
    SHT31_STATUS_TIMEOUT,             /** The configured I2C driver timeout expired. */
    SHT31_STATUS_COMMUNICATION_ERROR, /** The I2C dependency could not complete a transfer. */
    SHT31_STATUS_CRC_ERROR            /** At least one measurement word failed its CRC check. */
} sht31_status_t;

/**
 * @brief Minimum conversion wait after start_measurement() returns, in milliseconds.
 * @details Covers the maximum high-repeatability conversion time, including at
 * supply voltages below 2.4 V. Round up for the caller's timer resolution and
 * account for tick phase; a nominal delay of this many ticks can finish early.
 */
enum {
    SHT31_MEASUREMENT_TIME_MS = 16
};

/**
 * @brief One CRC-validated temperature and relative-humidity pair.
 * @details Values use the datasheet conversion, rounded to the nearest integer.
 * Representation resolution does not imply corresponding measurement accuracy.
 */
typedef struct {
    int32_t temperature_millicelsius; /** Temperature in 0.001 degrees Celsius; e.g. 24800 means 24.8 C. */
    uint32_t humidity_millipercent;   /** Relative humidity in 0.001 percent RH; e.g. 58300 means 58.3 %RH. */
} sht31_sample_t;

/**
 * @brief Initialize the complete context and disable the sensor's heater.
 *
 * @details No prior zeroing of sensor is required. Invalid arguments leave it
 * unchanged. A valid attempt replaces the context, which remains unavailable
 * unless the heater-disable I2C command succeeds. Initialization may be retried.
 * Supply dependencies and context storage through sht31_config.h.
 *
 * The board must provide stable power and a completed power-up/reset. This
 * component exclusively owns the sensor in single-shot mode from its idle
 * power-up state; taking over an externally configured periodic acquisition
 * requires stopping/resetting that acquisition first. Before reinitializing a
 * live sensor, allow any previous conversion to finish. init() does not reset
 * the sensor, construct the I2C bus, or reserve dynamic/arena storage.
 *
 * All calls must be serialized in normal execution context, including access to
 * the shared I2C bus. No ISR support, synchronization, scheduler, or delay is
 * provided. Transfers are synchronous and use the bus's configured timeout;
 * this does not guarantee a strict deadline for the whole call. The context and
 * borrowed bus must outlive their uses. Do not modify context fields or overlap
 * it with arguments/outputs. Other calls require a context previously passed to
 * init(), or a zero-initialized context which returns NOT_INITIALIZED.
 *
 * @param[out] sensor Required caller-owned context.
 * @param[in] config Required bus handle and unshifted address.
 * @return OK, INVALID_ARGUMENT, NOT_INITIALIZED, NOT_SUPPORTED, TIMEOUT, or
 * COMMUNICATION_ERROR, according to validation and the I2C result.
 */
sht31_status_t sht31_init(sht31_t *sensor, const sht31_config_t *config);

/**
 * @brief Start one high-repeatability measurement without clock stretching.
 *
 * @details Writes command 0x2400 and returns without waiting for conversion.
 * After success, wait at least SHT31_MEASUREMENT_TIME_MS from this call's return
 * before read_measurement(). The caller measures that elapsed time and can do
 * other work in between. The component never reads the clock or starts a timer.
 *
 * A second start before reading returns INVALID_STATE without I2C traffic. A
 * failed start leaves no pending read, but the command may have reached the
 * device: allow the full conversion interval before a new start or init().
 * Retry timing and preservation of the previous valid sample belong to the
 * caller; the component stores no previous sample and retries no transfers.
 *
 * @param[in,out] sensor Required initialized context with no pending measurement.
 * @return Component status, including INVALID_STATE if a measurement is pending.
 */
sht31_status_t sht31_start_measurement(sht31_t *sensor);

/**
 * @brief Read six bytes, validate both CRCs, and convert a pending measurement.
 *
 * @details Call only after the documented conversion wait. An early read can
 * receive a NACK, reported as COMMUNICATION_ERROR; the HAL cannot distinguish
 * that condition from other bus failures. No readiness polling is performed.
 *
 * out_sample is modified only on complete success. Once an actual read is
 * attempted, the pending measurement is consumed, including on I2C or CRC
 * failure: start a fresh measurement for the next acquisition. Argument or
 * lifecycle validation errors perform no I2C traffic and preserve pending work.
 *
 * @param[in,out] sensor Required initialized context with a pending measurement.
 * @param[out] out_sample Required independent output, written only on success.
 * @return Component status, including INVALID_STATE without a pending measurement
 * and CRC_ERROR for a corrupted temperature or humidity word.
 */
sht31_status_t sht31_read_measurement(sht31_t *sensor, sht31_sample_t *out_sample);

#ifdef __cplusplus
}
#endif

#endif
