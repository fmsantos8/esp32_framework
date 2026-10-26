/**
 * @file test_app_telemetry.c
 * @author Fabricio Santos (fmsantos8@ucs.br)
 * @brief Telemetry queue scenarios using the real component with simulated UART and time.
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cmp_telemetry_config.h"
#include "telemetry/app_telemetry.h"

#define CHECK(condition)                                                   \
    do {                                                                   \
        if (!(condition)) {                                                \
            fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #condition); \
            exit(EXIT_FAILURE);                                            \
        }                                                                  \
    } while (0)

/** @brief UART behavior and the latest requested record. */
struct hal_uart {
    hal_status_t status;    /** Driver result. */
    uint32_t accepted;      /** Accepted prefix length when a write fails. */
    uint32_t delay;         /** Time consumed by one write, in milliseconds. */
    uint32_t calls;         /** Number of driver calls, including verification writes. */
    char record[128];       /** Latest requested record, independent of accepted bytes. */
};

/** @brief Component initialized externally with a simulated UART. */
typedef struct {
    struct hal_uart uart;   /** Simulated UART. */
    telemetry_t component;  /** Real telemetry component. */
} fixture_t;

static uint32_t simulated_ticks;
static uint32_t clock_reads;

static uint32_t get_ticks(void) {
    ++clock_reads;
    return simulated_ticks;
}

hal_status_t hal_uart_write(hal_uart_t *uart, const uint8_t *buffer, uint32_t size, uint32_t *transmitted_count) {
    CHECK(uart != NULL && transmitted_count != NULL);
    CHECK(buffer != NULL || size == 0);
    CHECK(size < sizeof(uart->record));
    ++uart->calls;
    simulated_ticks += uart->delay;
    if (size != 0) {
        memcpy(uart->record, buffer, size);
    }
    uart->record[size] = '\0';
    *transmitted_count = uart->status == HAL_STATUS_OK ? size : (uart->accepted < size ? uart->accepted : size);
    return uart->status;
}

static void init_fixture(fixture_t *fixture) {
    *fixture = (fixture_t){.uart.status = HAL_STATUS_OK};
    simulated_ticks = 0;
    clock_reads = 0;
    const telemetry_config_t config = {.uart = &fixture->uart};
    CHECK(cmp_telemetry_init(&fixture->component, &config) == TELEMETRY_STATUS_OK);
    CHECK(fixture->uart.calls == 1);
    fixture->uart.calls = 0;
    CHECK(app_telemetry_init(get_ticks));
    CHECK(fixture->uart.calls == 0 && clock_reads == 0);
}

static void expect_run(fixture_t *fixture, uint32_t ticks, const char *record) {
    simulated_ticks = ticks;
    const uint32_t previous_calls = fixture->uart.calls;
    app_telemetry_run(&fixture->component);
    CHECK(fixture->uart.calls == previous_calls + (record != NULL ? 1U : 0U));
    if (record != NULL) {
        CHECK(strcmp(fixture->uart.record, record) == 0);
    }
}

static void expect_sensor_error(fixture_t *fixture, uint32_t ticks, uint32_t code) {
    char record[80];
    snprintf(record, sizeof(record), "ERROR;source=SHT31D;code=%lu\r\n", (unsigned long)code);
    expect_run(fixture, ticks, record);
}

static uint32_t fill_queue(void) {
    uint32_t accepted = 0;
    while (app_telemetry_report_error(APP_TELEMETRY_SOURCE_SENSOR, (int32_t)accepted)) {
        ++accepted;
        CHECK(accepted < 64);
    }
    CHECK(accepted > 0);
    return accepted;
}

static void test_run_before_init(void) {
    fixture_t fixture = {0};
    const telemetry_sample_t sample = {.temperature = 24800, .humidity = 58300};
    CHECK(!app_telemetry_init(NULL));
    CHECK(!app_telemetry_report_sample(&sample));
    CHECK(!app_telemetry_report_error(APP_TELEMETRY_SOURCE_SENSOR, 6));
    app_telemetry_run(NULL);
    app_telemetry_run(&fixture.component);
    CHECK(fixture.uart.calls == 0 && clock_reads == 0);
}

static void test_fifo_and_copy(void) {
    fixture_t fixture;
    init_fixture(&fixture);
    telemetry_sample_t sample = {.temperature = 24800, .humidity = 58300};
    CHECK(app_telemetry_report_sample(&sample));
    CHECK(app_telemetry_report_error(APP_TELEMETRY_SOURCE_SENSOR, 7));
    CHECK(app_telemetry_report_error(APP_TELEMETRY_SOURCE_DISPLAY, -12));
    sample = (telemetry_sample_t){.temperature = -5300, .humidity = 42700};
    CHECK(app_telemetry_report_sample(&sample));
    sample = (telemetry_sample_t){0};
    CHECK(fixture.uart.calls == 0 && clock_reads == 0);
    expect_run(&fixture, 0, "SAMPLE;temperature_mC=24800;humidity_mpercent=58300\r\n");
    expect_run(&fixture, 0, "ERROR;source=SHT31D;code=7\r\n");
    expect_run(&fixture, 0, "ERROR;source=SSD1306;code=-12\r\n");
    expect_run(&fixture, 0, "SAMPLE;temperature_mC=-5300;humidity_mpercent=42700\r\n");
    expect_run(&fixture, 0, NULL);
}

static void test_duplicate_errors(void) {
    fixture_t fixture;
    init_fixture(&fixture);
    CHECK(app_telemetry_report_error(APP_TELEMETRY_SOURCE_SENSOR, 6));
    CHECK(app_telemetry_report_error(APP_TELEMETRY_SOURCE_SENSOR, 6));
    CHECK(app_telemetry_report_error(APP_TELEMETRY_SOURCE_DISPLAY, 6));
    expect_run(&fixture, 0, "ERROR;source=SHT31D;code=6\r\n");
    expect_run(&fixture, 0, "ERROR;source=SHT31D;code=6\r\n");
    expect_run(&fixture, 0, "ERROR;source=SSD1306;code=6\r\n");
    expect_run(&fixture, 0, NULL);
}

static void test_value_ranges(void) {
    fixture_t fixture;
    init_fixture(&fixture);
    const telemetry_sample_t sample = {.temperature = INT32_MIN, .humidity = UINT32_MAX};
    CHECK(app_telemetry_report_sample(&sample));
    CHECK(app_telemetry_report_error(APP_TELEMETRY_SOURCE_SENSOR, INT32_MIN));
    CHECK(app_telemetry_report_error(APP_TELEMETRY_SOURCE_DISPLAY, INT32_MAX));
    expect_run(&fixture, 0, "SAMPLE;temperature_mC=-2147483648;humidity_mpercent=4294967295\r\n");
    expect_run(&fixture, 0, "ERROR;source=SHT31D;code=-2147483648\r\n");
    expect_run(&fixture, 0, "ERROR;source=SSD1306;code=2147483647\r\n");
}

static void test_queue_overflow(void) {
    fixture_t fixture;
    init_fixture(&fixture);
    const uint32_t capacity = fill_queue();
    const telemetry_sample_t sample = {0};
    CHECK(!app_telemetry_report_sample(&sample));
    CHECK(fixture.uart.calls == 0 && clock_reads == 0);
    expect_sensor_error(&fixture, 0, 0);
    CHECK(app_telemetry_report_error(APP_TELEMETRY_SOURCE_SENSOR, 999));
    for (uint32_t code = 1; code < capacity; ++code) {
        expect_sensor_error(&fixture, 0, code);
    }
    expect_sensor_error(&fixture, 0, 999);
    expect_run(&fixture, 0, NULL);
}

static void test_queue_wrap(void) {
    fixture_t fixture;
    init_fixture(&fixture);
    for (uint32_t cycle = 0; cycle < 32; ++cycle) {
        for (uint32_t item = 0; item < 3; ++item) {
            CHECK(app_telemetry_report_error(APP_TELEMETRY_SOURCE_SENSOR, (int32_t)(cycle * 4 + item)));
        }
        expect_sensor_error(&fixture, 0, cycle * 4);
        expect_sensor_error(&fixture, 0, cycle * 4 + 1);
        CHECK(app_telemetry_report_error(APP_TELEMETRY_SOURCE_SENSOR, (int32_t)(cycle * 4 + 3)));
        expect_sensor_error(&fixture, 0, cycle * 4 + 2);
        expect_sensor_error(&fixture, 0, cycle * 4 + 3);
        expect_run(&fixture, 0, NULL);
    }
}

static void test_zero_byte_retry(void) {
    static const hal_status_t failures[] = {HAL_STATUS_TIMEOUT, HAL_STATUS_ERROR};
    for (uint32_t i = 0; i < sizeof(failures) / sizeof(failures[0]); ++i) {
        fixture_t fixture;
        init_fixture(&fixture);
        CHECK(app_telemetry_report_error(APP_TELEMETRY_SOURCE_SENSOR, 6));
        CHECK(app_telemetry_report_error(APP_TELEMETRY_SOURCE_DISPLAY, 5));
        fixture.uart.status = failures[i];
        fixture.uart.delay = 37;
        expect_run(&fixture, 10, "ERROR;source=SHT31D;code=6\r\n");
        CHECK(simulated_ticks == 47);
        expect_run(&fixture, 1046, NULL);
        fixture.uart.status = HAL_STATUS_OK;
        fixture.uart.delay = 0;
        expect_run(&fixture, 1047, "ERROR;source=SHT31D;code=6\r\n");
        expect_run(&fixture, 1047, "ERROR;source=SSD1306;code=5\r\n");
        expect_run(&fixture, 1047, NULL);
    }
}

static void test_partial_write_discard(void) {
    static const hal_status_t failures[] = {HAL_STATUS_TIMEOUT, HAL_STATUS_ERROR};
    static const uint32_t prefixes[] = {5, UINT32_MAX};
    for (uint32_t i = 0; i < sizeof(failures) / sizeof(failures[0]); ++i) {
        for (uint32_t prefix = 0; prefix < sizeof(prefixes) / sizeof(prefixes[0]); ++prefix) {
            fixture_t fixture;
            init_fixture(&fixture);
            CHECK(app_telemetry_report_error(APP_TELEMETRY_SOURCE_SENSOR, 6));
            CHECK(app_telemetry_report_error(APP_TELEMETRY_SOURCE_DISPLAY, 5));
            fixture.uart.status = failures[i];
            fixture.uart.accepted = prefixes[prefix];
            expect_run(&fixture, 0, "ERROR;source=SHT31D;code=6\r\n");
            expect_run(&fixture, 999, NULL);
            fixture.uart.status = HAL_STATUS_OK;
            expect_run(&fixture, 1000, "ERROR;source=SSD1306;code=5\r\n");
            expect_run(&fixture, 1000, NULL);
        }
    }
}

static void test_permanent_failures(void) {
    static const hal_status_t failures[] = {HAL_STATUS_INVALID_ARGUMENT, HAL_STATUS_NOT_INITIALIZED, HAL_STATUS_NOT_SUPPORTED};
    for (uint32_t i = 0; i < sizeof(failures) / sizeof(failures[0]); ++i) {
        fixture_t fixture;
        init_fixture(&fixture);
        CHECK(app_telemetry_report_error(APP_TELEMETRY_SOURCE_SENSOR, 6));
        CHECK(app_telemetry_report_error(APP_TELEMETRY_SOURCE_DISPLAY, 5));
        fixture.uart.status = failures[i];
        expect_run(&fixture, 0, "ERROR;source=SHT31D;code=6\r\n");
        expect_run(&fixture, 999, NULL);
        fixture.uart.status = HAL_STATUS_OK;
        expect_run(&fixture, 1000, "ERROR;source=SSD1306;code=5\r\n");
        expect_run(&fixture, 1000, NULL);
    }
}

static void test_component_unavailable(void) {
    fixture_t fixture;
    init_fixture(&fixture);
    fixture.component = (telemetry_t){0};
    CHECK(app_telemetry_report_error(APP_TELEMETRY_SOURCE_SENSOR, 6));
    CHECK(app_telemetry_report_error(APP_TELEMETRY_SOURCE_DISPLAY, 5));
    expect_run(&fixture, 0, NULL);
    const telemetry_config_t config = {.uart = &fixture.uart};
    CHECK(cmp_telemetry_init(&fixture.component, &config) == TELEMETRY_STATUS_OK);
    expect_run(&fixture, 999, NULL);
    expect_run(&fixture, 1000, "ERROR;source=SSD1306;code=5\r\n");
    expect_run(&fixture, 1000, NULL);
}

static void test_invalid_arguments(void) {
    fixture_t fixture;
    init_fixture(&fixture);
    CHECK(!app_telemetry_report_sample(NULL));
    CHECK(!app_telemetry_report_error((telemetry_source_t)-1, 5));
    CHECK(!app_telemetry_report_error((telemetry_source_t)2, 5));
    CHECK(app_telemetry_report_error(APP_TELEMETRY_SOURCE_SENSOR, 6));
    CHECK(!app_telemetry_init(NULL));
    app_telemetry_run(NULL);
    CHECK(fixture.uart.calls == 0 && clock_reads == 0);
    expect_run(&fixture, 0, "ERROR;source=SHT31D;code=6\r\n");
    expect_run(&fixture, 0, NULL);
}

static void test_reinitialization(void) {
    fixture_t fixture;
    init_fixture(&fixture);
    fill_queue();
    CHECK(app_telemetry_init(get_ticks));
    CHECK(fixture.uart.calls == 0);
    expect_run(&fixture, 0, NULL);
    CHECK(app_telemetry_report_error(APP_TELEMETRY_SOURCE_SENSOR, 6));
    fixture.uart.status = HAL_STATUS_TIMEOUT;
    expect_run(&fixture, 0, "ERROR;source=SHT31D;code=6\r\n");
    CHECK(app_telemetry_init(get_ticks));
    CHECK(app_telemetry_report_error(APP_TELEMETRY_SOURCE_DISPLAY, 5));
    fixture.uart.status = HAL_STATUS_OK;
    expect_run(&fixture, 0, "ERROR;source=SSD1306;code=5\r\n");
    expect_run(&fixture, 0, NULL);
}

static void test_tick_wrap(void) {
    fixture_t fixture;
    init_fixture(&fixture);
    CHECK(app_telemetry_report_error(APP_TELEMETRY_SOURCE_SENSOR, 6));
    fixture.uart.status = HAL_STATUS_TIMEOUT;
    const uint32_t start = UINT32_MAX - 100U;
    expect_run(&fixture, start, "ERROR;source=SHT31D;code=6\r\n");
    expect_run(&fixture, start + 999U, NULL);
    fixture.uart.status = HAL_STATUS_OK;
    expect_run(&fixture, start + 1000U, "ERROR;source=SHT31D;code=6\r\n");
    expect_run(&fixture, start + 1000U, NULL);
}

static void test_repeated_uart_failures(void) {
    fixture_t fixture;
    init_fixture(&fixture);
    CHECK(app_telemetry_report_error(APP_TELEMETRY_SOURCE_SENSOR, 6));
    fixture.uart.status = HAL_STATUS_ERROR;
    for (uint32_t attempt = 0; attempt < 5; ++attempt) {
        expect_run(&fixture, attempt * 1000U, "ERROR;source=SHT31D;code=6\r\n");
        expect_run(&fixture, attempt * 1000U + 999U, NULL);
    }
    fixture.uart.status = HAL_STATUS_OK;
    expect_run(&fixture, 5000, "ERROR;source=SHT31D;code=6\r\n");
    expect_run(&fixture, 5000, NULL);
}

static void test_queue_full_retry(void) {
    fixture_t fixture;
    init_fixture(&fixture);
    CHECK(app_telemetry_report_error(APP_TELEMETRY_SOURCE_SENSOR, 99));
    fixture.uart.status = HAL_STATUS_TIMEOUT;
    expect_sensor_error(&fixture, 0, 99);
    const uint32_t added = fill_queue();
    expect_run(&fixture, 999, NULL);
    fixture.uart.status = HAL_STATUS_OK;
    expect_sensor_error(&fixture, 1000, 99);
    CHECK(app_telemetry_report_error(APP_TELEMETRY_SOURCE_SENSOR, 999));
    for (uint32_t code = 0; code < added; ++code) {
        expect_sensor_error(&fixture, 1000, code);
    }
    expect_sensor_error(&fixture, 1000, 999);
    expect_run(&fixture, 1000, NULL);
}

int main(int argc, char **argv) {
    static const struct {
        const char *name;
        void (*run)(void);
    } scenarios[] = {
        {"run_before_init", test_run_before_init},
        {"fifo_and_copy", test_fifo_and_copy},
        {"duplicate_errors", test_duplicate_errors},
        {"value_ranges", test_value_ranges},
        {"queue_overflow", test_queue_overflow},
        {"queue_wrap", test_queue_wrap},
        {"zero_byte_retry", test_zero_byte_retry},
        {"partial_write_discard", test_partial_write_discard},
        {"permanent_failures", test_permanent_failures},
        {"component_unavailable", test_component_unavailable},
        {"invalid_arguments", test_invalid_arguments},
        {"reinitialization", test_reinitialization},
        {"tick_wrap", test_tick_wrap},
        {"repeated_uart_failures", test_repeated_uart_failures},
        {"queue_full_retry", test_queue_full_retry},
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
