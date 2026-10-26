/**
 * @file test_app_status_led.c
 * @author Fabricio Santos (fmsantos8@ucs.br)
 * @brief Status LED scenarios using the real component and simulated GPIO and time.
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "status_led/app_status_led.h"
#include "cmp_led_config.h"

#define CHECK(condition)                                                  \
    do {                                                                  \
        if (!(condition)) {                                               \
            fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #condition); \
            exit(EXIT_FAILURE);                                           \
        }                                                                 \
    } while (0)

/** @brief GPIO state supplied only by the host test. */
struct hal_gpio {
    bool level;           /** Simulated physical output level. */
    bool requested_level; /** Last requested physical level. */
    hal_status_t status;  /** Result returned by the simulated driver. */
    uint32_t write_count; /** Number of attempted driver writes. */
};

/** @brief Component and output storage for one simulated LED. */
typedef struct {
    struct hal_gpio gpio; /** Simulated GPIO. */
    led_t component;      /** Real LED component. */
    bool active_level;    /** Electrical level that illuminates the LED. */
} fixture_t;

static uint32_t simulated_ticks;
static uint32_t clock_calls;

static uint32_t get_ticks(void) {
    ++clock_calls;
    return simulated_ticks;
}

hal_status_t hal_gpio_write(hal_gpio_t *gpio, bool level) {
    if (gpio == NULL) {
        return HAL_STATUS_INVALID_ARGUMENT;
    }
    ++gpio->write_count;
    gpio->requested_level = level;
    if (gpio->status == HAL_STATUS_OK) {
        gpio->level = level;
    }
    return gpio->status;
}

static void init_component(fixture_t *fixture, bool active_level) {
    *fixture = (fixture_t){
        .gpio = {.status = HAL_STATUS_OK},
        .active_level = active_level,
    };
    simulated_ticks = 0;
    clock_calls = 0;
    const led_config_t config = {
        .gpio = &fixture->gpio,
        .active_level = active_level,
    };
    CHECK(cmp_led_init(&fixture->component, &config) == LED_STATUS_OK);
    CHECK(fixture->gpio.level == !active_level);
}

static void init_fixture(fixture_t *fixture, bool active_level) {
    init_component(fixture, active_level);
    const uint32_t previous_writes = fixture->gpio.write_count;
    CHECK(app_status_led_init(get_ticks));
    CHECK(fixture->gpio.write_count == previous_writes);
    CHECK(clock_calls == 0);
}

static void expect_run(fixture_t *fixture, uint32_t ticks, bool on, bool write) {
    simulated_ticks = ticks;
    const uint32_t previous_writes = fixture->gpio.write_count;
    app_status_led_run(&fixture->component);
    CHECK(fixture->gpio.write_count == previous_writes + (write ? 1U : 0U));
    CHECK(fixture->gpio.level == (on ? fixture->active_level : !fixture->active_level));
}

static void test_run_before_init(void) {
    fixture_t fixture;
    init_component(&fixture, true);
    const uint32_t previous_writes = fixture.gpio.write_count;
    app_status_led_set_ready(true);
    app_status_led_set_i2c_error(APP_STATUS_LED_I2C_SENSOR, true);
    CHECK(!app_status_led_init(NULL));
    app_status_led_run(NULL);
    app_status_led_run(&fixture.component);
    CHECK(fixture.gpio.write_count == previous_writes);
    CHECK(clock_calls == 0);
}

static void test_startup_normal(void) {
    fixture_t fixture;
    init_fixture(&fixture, true);
    expect_run(&fixture, 0, false, true);
    expect_run(&fixture, 499, false, false);
    app_status_led_set_ready(true);
    expect_run(&fixture, 500, true, true);
    expect_run(&fixture, 1000, true, false);
    app_status_led_set_ready(false);
    expect_run(&fixture, 1500, false, true);
    expect_run(&fixture, 2000, false, false);
}

static void test_blink_recovery(void) {
    fixture_t fixture;
    init_fixture(&fixture, false);
    app_status_led_set_ready(true);
    expect_run(&fixture, 0, true, true);
    app_status_led_set_i2c_error(APP_STATUS_LED_I2C_SENSOR, true);
    expect_run(&fixture, 100, false, true);
    expect_run(&fixture, 599, false, false);
    expect_run(&fixture, 600, true, true);
    expect_run(&fixture, 1099, true, false);
    expect_run(&fixture, 1100, false, true);
    app_status_led_set_i2c_error(APP_STATUS_LED_I2C_SENSOR, false);
    expect_run(&fixture, 1101, true, true);
    expect_run(&fixture, 9999, true, false);
}

static void test_blink_restart(void) {
    fixture_t fixture;
    init_fixture(&fixture, true);
    app_status_led_set_ready(true);
    expect_run(&fixture, 0, true, true);
    app_status_led_set_i2c_error(APP_STATUS_LED_I2C_SENSOR, true);
    expect_run(&fixture, 10000, false, true);
    expect_run(&fixture, 10499, false, false);
    expect_run(&fixture, 10500, true, true);

    // Recovery must reset the sequence even when the output remains on.
    app_status_led_set_i2c_error(APP_STATUS_LED_I2C_SENSOR, false);
    expect_run(&fixture, 10501, true, false);
    app_status_led_set_i2c_error(APP_STATUS_LED_I2C_SENSOR, true);
    expect_run(&fixture, 10502, false, true);
    expect_run(&fixture, 11001, false, false);
    expect_run(&fixture, 11002, true, true);
    expect_run(&fixture, 11502, false, true);

    app_status_led_set_i2c_error(APP_STATUS_LED_I2C_SENSOR, false);
    expect_run(&fixture, 11503, true, true);
    app_status_led_set_i2c_error(APP_STATUS_LED_I2C_DISPLAY, true);
    expect_run(&fixture, 20000, false, true);
    expect_run(&fixture, 20499, false, false);
    expect_run(&fixture, 20500, true, true);
}

static void test_independent_i2c_errors(void) {
    static const app_status_led_i2c_source_t recovery_order[] = {
        APP_STATUS_LED_I2C_SENSOR,
        APP_STATUS_LED_I2C_DISPLAY,
    };
    for (uint32_t i = 0; i < sizeof(recovery_order) / sizeof(recovery_order[0]); ++i) {
        fixture_t fixture;
        init_fixture(&fixture, true);
        app_status_led_set_ready(true);
        expect_run(&fixture, 0, true, true);
        app_status_led_set_i2c_error(APP_STATUS_LED_I2C_SENSOR, true);
        expect_run(&fixture, 100, false, true);
        app_status_led_set_i2c_error(APP_STATUS_LED_I2C_DISPLAY, true);
        expect_run(&fixture, 200, false, false);
        expect_run(&fixture, 600, true, true);

        app_status_led_set_i2c_error(recovery_order[i], false);
        expect_run(&fixture, 650, true, false);
        expect_run(&fixture, 1100, false, true);
        app_status_led_set_i2c_error(recovery_order[1U - i], false);
        expect_run(&fixture, 1101, true, true);
        expect_run(&fixture, 2000, true, false);
    }
}

static void test_startup_i2c_error(void) {
    fixture_t fixture;
    init_fixture(&fixture, true);
    expect_run(&fixture, 0, false, true);
    app_status_led_set_i2c_error(APP_STATUS_LED_I2C_DISPLAY, true);
    expect_run(&fixture, 10, false, false);
    expect_run(&fixture, 509, false, false);
    expect_run(&fixture, 510, true, true);
    app_status_led_set_ready(true);
    expect_run(&fixture, 511, true, false);
    app_status_led_set_ready(false);
    expect_run(&fixture, 512, true, false);
    app_status_led_set_i2c_error(APP_STATUS_LED_I2C_DISPLAY, false);
    expect_run(&fixture, 520, false, true);
    app_status_led_set_ready(true);
    expect_run(&fixture, 521, true, true);
}

static void test_repeated_notifications(void) {
    fixture_t fixture;
    init_fixture(&fixture, true);
    app_status_led_set_ready(true);
    expect_run(&fixture, 0, true, true);
    app_status_led_set_i2c_error(APP_STATUS_LED_I2C_SENSOR, true);
    expect_run(&fixture, 100, false, true);
    for (uint32_t ticks = 200; ticks <= 500; ticks += 100) {
        app_status_led_set_i2c_error(APP_STATUS_LED_I2C_SENSOR, true);
        app_status_led_set_i2c_error(APP_STATUS_LED_I2C_DISPLAY, false);
        expect_run(&fixture, ticks, false, false);
    }
    expect_run(&fixture, 599, false, false);
    expect_run(&fixture, 600, true, true);
    expect_run(&fixture, 1100, false, true);
    app_status_led_set_i2c_error(APP_STATUS_LED_I2C_SENSOR, true);
    expect_run(&fixture, 1101, false, false);
    expect_run(&fixture, 1600, true, true);
}

static void test_tick_wrap_and_late_runs(void) {
    fixture_t fixture;
    init_fixture(&fixture, false);
    app_status_led_set_i2c_error(APP_STATUS_LED_I2C_SENSOR, true);
    expect_run(&fixture, UINT32_MAX - 249, false, true);
    expect_run(&fixture, 249, false, false);
    expect_run(&fixture, 250, true, true);
    expect_run(&fixture, 1750, false, true);
    expect_run(&fixture, 2249, false, false);
    expect_run(&fixture, 2250, true, true);
}

static void test_write_results_ignored(void) {
    static const hal_status_t failures[] = {
        HAL_STATUS_ERROR,
        HAL_STATUS_NOT_SUPPORTED,
        HAL_STATUS_NOT_INITIALIZED,
        HAL_STATUS_INVALID_ARGUMENT,
    };
    for (uint32_t i = 0; i < sizeof(failures) / sizeof(failures[0]); ++i) {
        fixture_t fixture;
        init_fixture(&fixture, true);
        app_status_led_set_ready(true);
        fixture.gpio.status = failures[i];
        expect_run(&fixture, 100, false, true);
        CHECK(fixture.gpio.requested_level == fixture.active_level);
        expect_run(&fixture, 600, false, false);
        fixture.gpio.status = HAL_STATUS_OK;
        expect_run(&fixture, 601, false, false);
        app_status_led_set_ready(false);
        expect_run(&fixture, 700, false, true);
        app_status_led_set_ready(true);
        expect_run(&fixture, 701, true, true);
        expect_run(&fixture, 2000, true, false);
    }

    fixture_t fixture;
    init_fixture(&fixture, false);
    app_status_led_set_ready(true);
    expect_run(&fixture, 0, true, true);
    app_status_led_set_i2c_error(APP_STATUS_LED_I2C_SENSOR, true);
    fixture.gpio.status = HAL_STATUS_ERROR;
    expect_run(&fixture, 100, true, true);
    CHECK(fixture.gpio.requested_level == !fixture.active_level);
    expect_run(&fixture, 101, true, false);
    fixture.gpio.status = HAL_STATUS_OK;
    expect_run(&fixture, 102, true, false);
    expect_run(&fixture, 600, true, true);
    fixture.gpio.status = HAL_STATUS_ERROR;
    expect_run(&fixture, 1100, true, true);
    fixture.gpio.status = HAL_STATUS_OK;
    app_status_led_set_i2c_error(APP_STATUS_LED_I2C_SENSOR, false);
    expect_run(&fixture, 1101, true, true);
}

static void test_component_unavailable(void) {
    fixture_t fixture = {
        .gpio = {.status = HAL_STATUS_OK},
        .active_level = true,
    };
    simulated_ticks = 0;
    CHECK(app_status_led_init(get_ticks));
    app_status_led_set_ready(true);
    app_status_led_run(&fixture.component);
    CHECK(fixture.gpio.write_count == 0);
    const led_config_t config = {
        .gpio = &fixture.gpio,
        .active_level = fixture.active_level,
    };
    CHECK(cmp_led_init(&fixture.component, &config) == LED_STATUS_OK);
    expect_run(&fixture, 1, false, false);
    expect_run(&fixture, 2, false, false);
    app_status_led_set_ready(false);
    expect_run(&fixture, 3, false, true);
    app_status_led_set_ready(true);
    expect_run(&fixture, 4, true, true);
}

static void test_invalid_arguments(void) {
    static const app_status_led_i2c_source_t invalid_sources[] = {
        (app_status_led_i2c_source_t)-1,
        (app_status_led_i2c_source_t)2,
        (app_status_led_i2c_source_t)255,
    };
    fixture_t fixture;
    init_fixture(&fixture, true);
    app_status_led_set_ready(true);
    expect_run(&fixture, 0, true, true);
    for (uint32_t i = 0; i < sizeof(invalid_sources) / sizeof(invalid_sources[0]); ++i) {
        app_status_led_set_i2c_error(invalid_sources[i], true);
    }
    expect_run(&fixture, 100, true, false);
    app_status_led_set_i2c_error(APP_STATUS_LED_I2C_SENSOR, true);
    expect_run(&fixture, 200, false, true);
    for (uint32_t i = 0; i < sizeof(invalid_sources) / sizeof(invalid_sources[0]); ++i) {
        app_status_led_set_i2c_error(invalid_sources[i], false);
    }
    const uint32_t previous_writes = fixture.gpio.write_count;
    const uint32_t previous_ticks = clock_calls;
    app_status_led_run(NULL);
    CHECK(fixture.gpio.write_count == previous_writes);
    CHECK(clock_calls == previous_ticks);
    expect_run(&fixture, 700, true, true);

    CHECK(!app_status_led_init(NULL));
    expect_run(&fixture, 1200, false, true);
    app_status_led_set_i2c_error(APP_STATUS_LED_I2C_SENSOR, false);
    expect_run(&fixture, 1201, true, true);
}

static void test_reinitialization(void) {
    fixture_t fixture;
    init_fixture(&fixture, true);
    app_status_led_set_ready(true);
    expect_run(&fixture, 0, true, true);
    app_status_led_set_i2c_error(APP_STATUS_LED_I2C_SENSOR, true);
    app_status_led_set_i2c_error(APP_STATUS_LED_I2C_DISPLAY, true);
    expect_run(&fixture, 10, false, true);
    expect_run(&fixture, 510, true, true);

    const uint32_t previous_writes = fixture.gpio.write_count;
    CHECK(app_status_led_init(get_ticks));
    CHECK(fixture.gpio.write_count == previous_writes);
    expect_run(&fixture, 520, false, true);
    app_status_led_set_ready(true);
    expect_run(&fixture, 521, true, true);
    app_status_led_set_i2c_error(APP_STATUS_LED_I2C_DISPLAY, true);
    expect_run(&fixture, 1000, false, true);
    expect_run(&fixture, 1499, false, false);
    expect_run(&fixture, 1500, true, true);
}

int main(int argc, char **argv) {
    static const struct {
        const char *name;
        void (*run)(void);
    } scenarios[] = {
        {"run_before_init", test_run_before_init},
        {"startup_normal", test_startup_normal},
        {"blink_recovery", test_blink_recovery},
        {"blink_restart", test_blink_restart},
        {"independent_i2c_errors", test_independent_i2c_errors},
        {"startup_i2c_error", test_startup_i2c_error},
        {"repeated_notifications", test_repeated_notifications},
        {"tick_wrap_and_late_runs", test_tick_wrap_and_late_runs},
        {"write_results_ignored", test_write_results_ignored},
        {"component_unavailable", test_component_unavailable},
        {"invalid_arguments", test_invalid_arguments},
        {"reinitialization", test_reinitialization},
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
