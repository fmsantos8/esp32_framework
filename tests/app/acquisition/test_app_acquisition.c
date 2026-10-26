/**
 * @file test_app_acquisition.c
 * @author Fabricio Santos (fmsantos8@ucs.br)
 * @brief Acquisition scenarios using the real sensor component and simulated I2C and time.
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "acquisition/app_acquisition.h"
#include "cmp_sht31d_config.h"
#include "status_led/app_status_led.h"
#include "telemetry/app_telemetry.h"

#define CHECK(condition)                                                  \
    do {                                                                  \
        if (!(condition)) {                                               \
            fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #condition); \
            exit(EXIT_FAILURE);                                           \
        }                                                                 \
    } while (0)

/** @brief Bus state supplied only by the host test. */
struct hal_i2c {
    hal_status_t write_status; /** Status returned by writes. */
    hal_status_t read_status;  /** Status returned by reads. */
    uint32_t write_count;      /** Number of attempted writes. */
    uint32_t read_count;       /** Number of attempted reads. */
    uint32_t write_delay;      /** Time consumed by one write, in milliseconds. */
    uint32_t read_delay;       /** Time consumed by one read, in milliseconds. */
    uint8_t bytes[6];          /** Measurement response, including both CRC bytes. */
    uint8_t command[2];        /** Last requested command. */
    uint8_t address;           /** Last addressed device. */
};

/** @brief Initialized sensor, bus, and last queried result. */
typedef struct {
    struct hal_i2c bus;              /** Simulated I2C bus. */
    sht31d_t sensor;                 /** Real sensor component. */
    acquisition_t result;           /** Result copied through the public getter. */
} fixture_t;

static uint32_t simulated_ticks;
static uint32_t led_update_count;
static uint32_t telemetry_event_count;
static const uint8_t cold_humid[] = {0x00, 0x00, 0x81, 0xFF, 0xFF, 0xAC};
static const uint8_t hot_dry[] = {0xFF, 0xFF, 0xAC, 0x00, 0x00, 0x81};

static uint32_t get_ticks(void) {
    return simulated_ticks;
}

hal_status_t hal_i2c_write(hal_i2c_t *i2c, uint8_t address, const uint8_t *buffer, uint32_t size) {
    CHECK(i2c != NULL && buffer != NULL);
    CHECK(address == 0x44 || address == 0x45);
    CHECK(size == 2);
    ++i2c->write_count;
    i2c->address = address;
    memcpy(i2c->command, buffer, 2);
    simulated_ticks += i2c->write_delay;
    return i2c->write_status;
}

hal_status_t hal_i2c_read(hal_i2c_t *i2c, uint8_t address, uint8_t *buffer, uint32_t size) {
    CHECK(i2c != NULL && buffer != NULL);
    CHECK(address == 0x44 || address == 0x45);
    CHECK(size == 6);
    ++i2c->read_count;
    i2c->address = address;
    memcpy(buffer, i2c->bytes, i2c->read_status == HAL_STATUS_OK ? 6 : 3);
    simulated_ticks += i2c->read_delay;
    return i2c->read_status;
}

void app_status_led_set_i2c_error(app_status_led_i2c_source_t source, bool active) {
    CHECK(source == APP_STATUS_LED_I2C_SENSOR);
    ++led_update_count;
    acquisition_t result;
    CHECK(app_acquisition_get_result(&result));
    CHECK(active == result.i2c_error);
}

bool app_telemetry_report_sample(const telemetry_sample_t *sample) {
    CHECK(sample != NULL);
    acquisition_t result;
    CHECK(app_acquisition_get_result(&result));
    CHECK(result.status == SHT31D_STATUS_OK && result.has_sample);
    CHECK(sample->temperature == result.sample.temperature && sample->humidity == result.sample.humidity);
    ++telemetry_event_count;
    return true;
}

bool app_telemetry_report_error(telemetry_source_t source, int32_t code) {
    CHECK(source == APP_TELEMETRY_SOURCE_SENSOR);
    acquisition_t result;
    CHECK(app_acquisition_get_result(&result));
    CHECK(result.status != SHT31D_STATUS_OK && code == (int32_t)result.status);
    ++telemetry_event_count;
    return true;
}

static void init_fixture(fixture_t *fixture) {
    *fixture = (fixture_t){
        .bus = {
            .write_status = HAL_STATUS_OK,
            .read_status = HAL_STATUS_OK,
        },
    };
    memcpy(fixture->bus.bytes, cold_humid, sizeof(cold_humid));
    memset(&fixture->sensor, 0xA5, sizeof(fixture->sensor));

    // Let a previous failed attempt finish its recovery wait before resetting the test clock.
    simulated_ticks += 2000;
    led_update_count = 0;
    telemetry_event_count = 0;
    CHECK(app_acquisition_init(get_ticks));
    simulated_ticks = 0;
    CHECK(fixture->bus.write_count == 0 && fixture->bus.read_count == 0);
    CHECK(led_update_count == 0);
    CHECK(telemetry_event_count == 0);

    // Composition initializes the sensor before handing it to acquisition.
    const sht31d_config_t config = {.i2c = &fixture->bus, .address = 0x44};
    CHECK(cmp_sht31d_init(&fixture->sensor, &config) == SHT31D_STATUS_OK);
    CHECK(fixture->bus.write_count == 1 && fixture->bus.read_count == 0);
    CHECK(fixture->bus.command[0] == 0x30 && fixture->bus.command[1] == 0x66);
    fixture->bus.write_count = 0;
}

static void expect_run(fixture_t *fixture, uint32_t ticks, uint32_t writes, uint32_t reads, uint32_t led_updates) {
    simulated_ticks = ticks;
    const uint32_t previous_writes = fixture->bus.write_count;
    const uint32_t previous_reads = fixture->bus.read_count;
    const uint32_t previous_updates = led_update_count;
    const uint32_t previous_events = telemetry_event_count;
    app_acquisition_run(&fixture->sensor);
    CHECK(fixture->bus.write_count == previous_writes + writes);
    CHECK(fixture->bus.read_count == previous_reads + reads);
    CHECK(led_update_count == previous_updates + led_updates);
    CHECK(telemetry_event_count == previous_events + led_updates);
    if (writes != 0) {
        CHECK(fixture->bus.command[0] == 0x24 && fixture->bus.command[1] == 0x00);
    }
    app_acquisition_get_result(&fixture->result);
}

static void expect_sample(const acquisition_t *result, int32_t temperature, uint32_t humidity, uint32_t acquired_at) {
    CHECK(result->has_sample);
    CHECK(result->sample.temperature == temperature);
    CHECK(result->sample.humidity == humidity);
    CHECK(result->acquired_at == acquired_at);
}

static void first_sample(fixture_t *fixture) {
    expect_run(fixture, 0, 1, 0, 0);
    expect_run(fixture, 16, 0, 1, 1);
    CHECK(fixture->result.status == SHT31D_STATUS_OK);
    expect_sample(&fixture->result, -45000, 100000, 16);
}

static void test_run_before_init(void) {
    fixture_t fixture = {0};
    CHECK(!app_acquisition_init(NULL));
    app_acquisition_run(&fixture.sensor);
    app_acquisition_run(NULL);
    CHECK(fixture.bus.write_count == 0 && fixture.bus.read_count == 0);
    CHECK(!app_acquisition_get_result(NULL));

    acquisition_t result;
    unsigned char before[sizeof(result)];
    memset(&result, 0xA5, sizeof(result));
    memcpy(before, &result, sizeof(result));
    CHECK(!app_acquisition_get_result(&result));
    CHECK(memcmp(before, &result, sizeof(result)) == 0);
}

static void test_periodic_samples(void) {
    fixture_t fixture;
    init_fixture(&fixture);
    acquisition_t snapshot;
    CHECK(!app_acquisition_get_result(&snapshot));
    expect_run(&fixture, 0, 1, 0, 0);
    CHECK(fixture.bus.command[0] == 0x24 && fixture.bus.command[1] == 0x00);
    expect_run(&fixture, 15, 0, 0, 0);
    expect_run(&fixture, 16, 0, 1, 1);
    CHECK(fixture.result.status == SHT31D_STATUS_OK);
    CHECK(!fixture.result.i2c_error);
    expect_sample(&fixture.result, -45000, 100000, 16);
    CHECK(app_acquisition_get_result(&snapshot));

    memcpy(fixture.bus.bytes, hot_dry, sizeof(hot_dry));
    expect_run(&fixture, 1999, 0, 0, 0);
    expect_run(&fixture, 2000, 1, 0, 0);
    expect_run(&fixture, 2015, 0, 0, 0);
    expect_run(&fixture, 2016, 0, 1, 1);
    expect_sample(&fixture.result, 130000, 0, 2016);
    CHECK(snapshot.sample.temperature == -45000 && snapshot.acquired_at == 16);
    CHECK(fixture.bus.address == 0x44);
}

static void test_uninitialized_sensor(void) {
    fixture_t fixture;
    init_fixture(&fixture);
    fixture.sensor = (sht31d_t){0};
    expect_run(&fixture, 0, 0, 0, 1);
    CHECK(fixture.result.status == SHT31D_STATUS_NOT_INITIALIZED);
    CHECK(!fixture.result.i2c_error && !fixture.result.has_sample);
    expect_run(&fixture, 1999, 0, 0, 0);
    expect_run(&fixture, 2000, 0, 0, 1);

    const sht31d_config_t config = {.i2c = &fixture.bus, .address = 0x44};
    CHECK(cmp_sht31d_init(&fixture.sensor, &config) == SHT31D_STATUS_OK);
    expect_run(&fixture, 3999, 0, 0, 0);
    expect_run(&fixture, 4000, 1, 0, 0);
    expect_run(&fixture, 4016, 0, 1, 1);
    CHECK(led_update_count == 3);
    CHECK(!fixture.result.i2c_error);
    expect_sample(&fixture.result, -45000, 100000, 4016);
}

static void test_start_failure(void) {
    fixture_t fixture;
    init_fixture(&fixture);
    fixture.bus.write_status = HAL_STATUS_ERROR;
    expect_run(&fixture, 100, 1, 0, 1);
    CHECK(fixture.result.status == SHT31D_STATUS_COMMUNICATION_ERROR);
    CHECK(fixture.result.i2c_error && !fixture.result.has_sample);
    expect_run(&fixture, 115, 0, 0, 0);
    expect_run(&fixture, 2099, 0, 0, 0);
    fixture.bus.write_status = HAL_STATUS_OK;
    expect_run(&fixture, 2100, 1, 0, 0);
    CHECK(fixture.result.i2c_error);
    expect_run(&fixture, 2116, 0, 1, 1);
    CHECK(!fixture.result.i2c_error);
}

static void test_read_failure_retains_sample(void) {
    fixture_t fixture;
    init_fixture(&fixture);
    first_sample(&fixture);
    memcpy(fixture.bus.bytes, hot_dry, sizeof(hot_dry));
    expect_run(&fixture, 2000, 1, 0, 0);
    fixture.bus.read_status = HAL_STATUS_TIMEOUT;
    expect_run(&fixture, 2016, 0, 1, 1);
    CHECK(fixture.result.status == SHT31D_STATUS_TIMEOUT);
    CHECK(fixture.result.i2c_error);
    expect_sample(&fixture.result, -45000, 100000, 16);
    expect_run(&fixture, 3999, 0, 0, 0);
    expect_run(&fixture, 4000, 1, 0, 0);
    CHECK(fixture.result.i2c_error);
    fixture.bus.read_status = HAL_STATUS_OK;
    expect_run(&fixture, 4016, 0, 1, 1);
    CHECK(!fixture.result.i2c_error);
    expect_sample(&fixture.result, 130000, 0, 4016);
}

static void test_crc_failures(void) {
    fixture_t fixture;
    init_fixture(&fixture);
    first_sample(&fixture);
    memcpy(fixture.bus.bytes, hot_dry, sizeof(hot_dry));
    fixture.bus.bytes[2] ^= 1U;
    expect_run(&fixture, 2000, 1, 0, 0);
    expect_run(&fixture, 2016, 0, 1, 1);
    CHECK(fixture.result.status == SHT31D_STATUS_CRC_ERROR);
    CHECK(!fixture.result.i2c_error);
    expect_sample(&fixture.result, -45000, 100000, 16);

    memcpy(fixture.bus.bytes, hot_dry, sizeof(hot_dry));
    fixture.bus.bytes[5] ^= 1U;
    expect_run(&fixture, 4000, 1, 0, 0);
    CHECK(fixture.bus.command[0] == 0x24 && fixture.bus.command[1] == 0x00);
    expect_run(&fixture, 4016, 0, 1, 1);
    CHECK(fixture.result.status == SHT31D_STATUS_CRC_ERROR);
    expect_sample(&fixture.result, -45000, 100000, 16);
    memcpy(fixture.bus.bytes, hot_dry, sizeof(hot_dry));
    expect_run(&fixture, 6000, 1, 0, 0);
    expect_run(&fixture, 6016, 0, 1, 1);
    expect_sample(&fixture.result, 130000, 0, 6016);
}

static void test_i2c_recovery_with_crc(void) {
    fixture_t fixture;
    init_fixture(&fixture);
    fixture.bus.write_status = HAL_STATUS_TIMEOUT;
    expect_run(&fixture, 0, 1, 0, 1);
    CHECK(fixture.result.i2c_error);
    fixture.bus.write_status = HAL_STATUS_OK;
    expect_run(&fixture, 2000, 1, 0, 0);
    fixture.bus.bytes[2] ^= 1U;
    expect_run(&fixture, 2016, 0, 1, 1);
    CHECK(fixture.result.status == SHT31D_STATUS_CRC_ERROR);
    CHECK(!fixture.result.has_sample && !fixture.result.i2c_error);
    memcpy(fixture.bus.bytes, cold_humid, sizeof(cold_humid));
    expect_run(&fixture, 4000, 1, 0, 0);
    expect_run(&fixture, 4016, 0, 1, 1);
    expect_sample(&fixture.result, -45000, 100000, 4016);
}

static void test_invalid_arguments(void) {
    fixture_t fixture;
    init_fixture(&fixture);
    CHECK(!app_acquisition_init(NULL));
    app_acquisition_run(NULL);
    CHECK(fixture.bus.write_count == 0 && fixture.bus.read_count == 0);
    CHECK(!app_acquisition_get_result(NULL));

    expect_run(&fixture, 0, 1, 0, 0);
    app_acquisition_run(NULL);
    CHECK(!app_acquisition_init(get_ticks));
    expect_run(&fixture, 16, 0, 1, 1);
    CHECK(led_update_count == 1);
}

static void test_driver_errors(void) {
    static const struct {
        hal_status_t driver_status;
        sht31d_status_t sensor_status;
        bool i2c_error;
    } cases[] = {
        {HAL_STATUS_INVALID_ARGUMENT, SHT31D_STATUS_INVALID_ARGUMENT, false},
        {HAL_STATUS_NOT_INITIALIZED, SHT31D_STATUS_NOT_INITIALIZED, false},
        {HAL_STATUS_NOT_SUPPORTED, SHT31D_STATUS_NOT_SUPPORTED, false},
        {HAL_STATUS_ERROR, SHT31D_STATUS_COMMUNICATION_ERROR, true},
        {HAL_STATUS_TIMEOUT, SHT31D_STATUS_TIMEOUT, true},
    };
    for (uint32_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        fixture_t fixture;
        init_fixture(&fixture);
        fixture.bus.write_status = cases[i].driver_status;
        expect_run(&fixture, 0, 1, 0, 1);
        CHECK(fixture.result.status == cases[i].sensor_status);
        CHECK(fixture.result.i2c_error == cases[i].i2c_error);
        CHECK(!fixture.result.has_sample);
    }
}

static void test_conversion_from_return(void) {
    fixture_t fixture;
    init_fixture(&fixture);
    fixture.bus.write_delay = 50;
    expect_run(&fixture, 100, 1, 0, 0);
    CHECK(simulated_ticks == 150);
    expect_run(&fixture, 165, 0, 0, 0);
    fixture.bus.read_delay = 7;
    expect_run(&fixture, 166, 0, 1, 1);
    expect_sample(&fixture.result, -45000, 100000, 173);
    expect_run(&fixture, 2149, 0, 0, 0);
    expect_run(&fixture, 2150, 1, 0, 0);
    expect_run(&fixture, 2215, 0, 0, 0);
    expect_run(&fixture, 2216, 0, 1, 1);
    expect_sample(&fixture.result, -45000, 100000, 2223);
}

static void test_tick_wrap(void) {
    fixture_t fixture;
    init_fixture(&fixture);
    expect_run(&fixture, UINT32_MAX - 7, 1, 0, 0);
    expect_run(&fixture, 7, 0, 0, 0);
    expect_run(&fixture, 8, 0, 1, 1);
    expect_sample(&fixture.result, -45000, 100000, 8);
    expect_run(&fixture, 1991, 0, 0, 0);
    expect_run(&fixture, 1992, 1, 0, 0);
    expect_run(&fixture, 2007, 0, 0, 0);
    expect_run(&fixture, 2008, 0, 1, 1);
}

static void test_late_runs(void) {
    fixture_t fixture;
    init_fixture(&fixture);
    expect_run(&fixture, 0, 1, 0, 0);
    expect_run(&fixture, 5000, 0, 1, 1);
    expect_sample(&fixture.result, -45000, 100000, 5000);
    expect_run(&fixture, 5000, 1, 0, 0);
    expect_run(&fixture, 5015, 0, 0, 0);
    expect_run(&fixture, 5016, 0, 1, 1);
}

static void test_result_readers(void) {
    fixture_t fixture;
    init_fixture(&fixture);
    first_sample(&fixture);

    acquisition_t first_reader;
    acquisition_t second_reader;
    CHECK(app_acquisition_get_result(&first_reader));
    CHECK(app_acquisition_get_result(&second_reader));
    expect_sample(&first_reader, -45000, 100000, 16);
    expect_sample(&second_reader, -45000, 100000, 16);

    // Changing one application's copy must not affect acquisition or other readers.
    first_reader.sample.temperature = 0;
    first_reader.has_sample = false;
    first_reader.i2c_error = true;
    CHECK(app_acquisition_get_result(&first_reader));
    expect_sample(&first_reader, -45000, 100000, 16);
    CHECK(!first_reader.i2c_error);
    CHECK(fixture.bus.write_count == 1 && fixture.bus.read_count == 1);
    CHECK(led_update_count == 1);

    memcpy(fixture.bus.bytes, hot_dry, sizeof(hot_dry));
    expect_run(&fixture, 2000, 1, 0, 0);
    CHECK(app_acquisition_get_result(&first_reader));
    expect_sample(&first_reader, -45000, 100000, 16);
    expect_run(&fixture, 2016, 0, 1, 1);
    CHECK(app_acquisition_get_result(&first_reader));
    expect_sample(&first_reader, 130000, 0, 2016);
    expect_sample(&second_reader, -45000, 100000, 16);
}

static void test_reinitialization(void) {
    fixture_t fixture;
    init_fixture(&fixture);
    first_sample(&fixture);
    const uint32_t previous_writes = fixture.bus.write_count;
    CHECK(app_acquisition_init(get_ticks));
    CHECK(fixture.bus.write_count == previous_writes);
    acquisition_t result;
    CHECK(!app_acquisition_get_result(&result));
    expect_run(&fixture, 100, 1, 0, 0);
    expect_run(&fixture, 116, 0, 1, 1);
    expect_sample(&fixture.result, -45000, 100000, 116);
}

static void test_pending_reinitialization(void) {
    fixture_t fixture;
    init_fixture(&fixture);
    expect_run(&fixture, 0, 1, 0, 0);
    CHECK(!app_acquisition_init(get_ticks));
    simulated_ticks = 16;
    CHECK(!app_acquisition_init(get_ticks));
    expect_run(&fixture, 16, 0, 1, 1);

    CHECK(app_acquisition_init(get_ticks));
    fixture.bus.write_status = HAL_STATUS_TIMEOUT;
    expect_run(&fixture, 100, 1, 0, 1);
    simulated_ticks = 115;
    CHECK(!app_acquisition_init(get_ticks));
    simulated_ticks = 116;
    CHECK(app_acquisition_init(get_ticks));
    fixture.bus.write_status = HAL_STATUS_OK;
    expect_run(&fixture, 116, 1, 0, 0);
    expect_run(&fixture, 132, 0, 1, 1);
}

int main(int argc, char **argv) {
    static const struct {
        const char *name;
        void (*run)(void);
    } scenarios[] = {
        {"run_before_init", test_run_before_init},
        {"periodic_samples", test_periodic_samples},
        {"uninitialized_sensor", test_uninitialized_sensor},
        {"start_failure", test_start_failure},
        {"read_failure_retains_sample", test_read_failure_retains_sample},
        {"crc_failures", test_crc_failures},
        {"i2c_recovery_with_crc", test_i2c_recovery_with_crc},
        {"invalid_arguments", test_invalid_arguments},
        {"driver_errors", test_driver_errors},
        {"conversion_from_return", test_conversion_from_return},
        {"tick_wrap", test_tick_wrap},
        {"late_runs", test_late_runs},
        {"result_readers", test_result_readers},
        {"reinitialization", test_reinitialization},
        {"pending_reinitialization", test_pending_reinitialization},
    };
    for (uint32_t i = 0; i < sizeof(scenarios) / sizeof(scenarios[0]); ++i) {
        if (argc == 1 || (argc == 2 && strcmp(argv[1], scenarios[i].name) == 0)) {
            scenarios[i].run();
            printf("PASS %s\n", scenarios[i].name);
            if (argc == 2) {
                return EXIT_SUCCESS;
            }
        }
    }
    return argc == 1 ? EXIT_SUCCESS : EXIT_FAILURE;
}
