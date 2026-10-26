/**
 * @file test_app_button.c
 * @author Fabricio Santos (fmsantos8@ucs.br)
 * @brief Button reading scenarios using the real component and simulated GPIO and time.
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "button/app_button.h"
#include "cmp_button_config.h"
#include "gui/app_gui.h"

#define CHECK(condition)                                                  \
    do {                                                                  \
        if (!(condition)) {                                               \
            fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #condition); \
            exit(EXIT_FAILURE);                                           \
        }                                                                 \
    } while (0)

/** @brief GPIO state supplied only by the host test. */
struct hal_gpio {
    bool level;          /** Simulated physical input level. */
    hal_status_t status;  /** Result returned by the simulated driver. */
    uint32_t read_count;  /** Number of attempted driver readings. */
};

/** @brief Component storage for one simulated input. */
typedef struct {
    struct hal_gpio gpio; /** Simulated GPIO. */
    button_t component;   /** Real button component. */
    bool active_level;    /** Component polarity. */
} fixture_t;

static uint32_t simulated_ticks;
static uint32_t screen_toggles;

void app_gui_toggle_screen(void) {
    ++screen_toggles;
}

static uint32_t get_ticks(void) {
    return simulated_ticks;
}

hal_status_t hal_gpio_read(hal_gpio_t *gpio, bool *level) {
    if (gpio == NULL || level == NULL) {
        return HAL_STATUS_INVALID_ARGUMENT;
    }
    ++gpio->read_count;
    if (gpio->status == HAL_STATUS_OK) {
        *level = gpio->level;
    }
    return gpio->status;
}

static void init_component(fixture_t *fixture, bool active_level, bool pressed) {
    *fixture = (fixture_t){
        .gpio = {
            .level = pressed ? active_level : !active_level,
            .status = HAL_STATUS_OK,
        },
        .active_level = active_level,
    };
    simulated_ticks = 0;
    const button_config_t component_config = {
        .gpio = &fixture->gpio,
        .active_level = active_level,
        .debounce = 30,
        .get_ticks = get_ticks,
    };
    CHECK(cmp_button_init(&fixture->component, &component_config) == BUTTON_STATUS_OK);
}

static void init_fixture(fixture_t *fixture, bool active_level, bool pressed) {
    init_component(fixture, active_level, pressed);
    screen_toggles = 0;
    const uint32_t previous_reads = fixture->gpio.read_count;
    app_button_init();
    CHECK(fixture->gpio.read_count == previous_reads);
    CHECK(screen_toggles == 0);
}

static void expect_run(fixture_t *fixture, uint32_t ticks, bool pressed, uint32_t expected_toggles) {
    simulated_ticks = ticks;
    fixture->gpio.level = pressed ? fixture->active_level : !fixture->active_level;
    const uint32_t previous_reads = fixture->gpio.read_count;
    app_button_run(&fixture->component);
    CHECK(fixture->gpio.read_count == previous_reads + 1);
    CHECK(screen_toggles == expected_toggles);
}

static void test_run_before_init(void) {
    fixture_t fixture;
    init_component(&fixture, false, true);
    const uint32_t previous_reads = fixture.gpio.read_count;
    app_button_run(NULL);
    app_button_run(&fixture.component);
    CHECK(fixture.gpio.read_count == previous_reads);
    CHECK(screen_toggles == 0);
}

static void test_press_hold_release(void) {
    fixture_t fixture;
    init_fixture(&fixture, false, false);
    expect_run(&fixture, 0, false, 0);
    expect_run(&fixture, 30, false, 0);
    expect_run(&fixture, 40, true, 0);
    expect_run(&fixture, 69, true, 0);
    expect_run(&fixture, 70, true, 1);
    expect_run(&fixture, 1000, true, 1);
    expect_run(&fixture, 1010, false, 1);
    expect_run(&fixture, 1040, false, 1);
    expect_run(&fixture, 1050, true, 1);
    expect_run(&fixture, 1080, true, 2);
}

static void test_contact_bounce(void) {
    fixture_t fixture;
    init_fixture(&fixture, false, false);
    expect_run(&fixture, 0, false, 0);
    expect_run(&fixture, 30, false, 0);
    expect_run(&fixture, 40, true, 0);
    expect_run(&fixture, 45, false, 0);
    expect_run(&fixture, 50, true, 0);
    expect_run(&fixture, 60, false, 0);
    expect_run(&fixture, 70, true, 0);
    expect_run(&fixture, 99, true, 0);
    expect_run(&fixture, 100, true, 1);
    expect_run(&fixture, 110, false, 1);
    expect_run(&fixture, 120, true, 1);
    expect_run(&fixture, 130, false, 1);
    expect_run(&fixture, 140, true, 1);
    expect_run(&fixture, 170, false, 1);
    expect_run(&fixture, 199, false, 1);
    expect_run(&fixture, 200, false, 1);
    expect_run(&fixture, 201, true, 1);
    expect_run(&fixture, 231, true, 2);
}

static void test_pressed_at_startup(void) {
    fixture_t fixture;
    init_fixture(&fixture, false, true);
    expect_run(&fixture, 0, true, 0);
    expect_run(&fixture, 29, true, 0);
    expect_run(&fixture, 30, true, 1);
    expect_run(&fixture, 100, true, 1);
}

static void test_failure_and_recovery(void) {
    fixture_t fixture;
    init_fixture(&fixture, false, true);
    expect_run(&fixture, 0, true, 0);
    expect_run(&fixture, 30, true, 1);
    fixture.gpio.status = HAL_STATUS_ERROR;
    expect_run(&fixture, 40, true, 1);
    expect_run(&fixture, 41, true, 1);

    fixture.gpio.status = HAL_STATUS_NOT_SUPPORTED;
    expect_run(&fixture, 42, true, 1);
    expect_run(&fixture, 43, true, 1);

    fixture.gpio.status = HAL_STATUS_OK;
    expect_run(&fixture, 50, true, 1);
    expect_run(&fixture, 80, true, 1);
    fixture.gpio.status = HAL_STATUS_NOT_SUPPORTED;
    expect_run(&fixture, 81, true, 1);
    expect_run(&fixture, 82, true, 1);
    fixture.gpio.status = HAL_STATUS_OK;
    expect_run(&fixture, 90, true, 1);
    expect_run(&fixture, 100, false, 1);
    expect_run(&fixture, 130, false, 1);
    expect_run(&fixture, 140, true, 1);
    expect_run(&fixture, 170, true, 2);
}

static void test_unobserved_release(void) {
    fixture_t fixture;
    init_fixture(&fixture, false, true);
    expect_run(&fixture, 0, true, 0);
    expect_run(&fixture, 30, true, 1);
    fixture.gpio.status = HAL_STATUS_ERROR;
    expect_run(&fixture, 40, false, 1);
    expect_run(&fixture, 70, false, 1);
    expect_run(&fixture, 80, true, 1);
    fixture.gpio.status = HAL_STATUS_OK;
    expect_run(&fixture, 90, true, 1);
    expect_run(&fixture, 120, true, 1);
    expect_run(&fixture, 130, false, 1);
    expect_run(&fixture, 160, false, 1);
    expect_run(&fixture, 170, true, 1);
    expect_run(&fixture, 200, true, 2);
}

static void test_failure_during_debounce(void) {
    fixture_t fixture;
    init_fixture(&fixture, false, false);
    expect_run(&fixture, 0, false, 0);
    expect_run(&fixture, 30, false, 0);
    expect_run(&fixture, 40, true, 0);
    fixture.gpio.status = HAL_STATUS_ERROR;
    expect_run(&fixture, 50, true, 0);
    fixture.gpio.status = HAL_STATUS_OK;
    expect_run(&fixture, 80, true, 0);
    expect_run(&fixture, 109, true, 0);
    expect_run(&fixture, 110, true, 1);
}

static void test_read_failures(void) {
    static const hal_status_t failures[] = {
        HAL_STATUS_INVALID_ARGUMENT,
        HAL_STATUS_NOT_INITIALIZED,
        HAL_STATUS_NOT_SUPPORTED,
        HAL_STATUS_ERROR,
    };
    for (uint32_t i = 0; i < sizeof(failures) / sizeof(failures[0]); ++i) {
        fixture_t fixture;
        init_fixture(&fixture, false, true);
        fixture.gpio.status = failures[i];
        expect_run(&fixture, 0, true, 0);
        expect_run(&fixture, 29, true, 0);

        fixture.gpio.status = HAL_STATUS_OK;
        expect_run(&fixture, 30, true, 0);
        expect_run(&fixture, 59, true, 0);
        expect_run(&fixture, 60, true, 1);
        expect_run(&fixture, 100, true, 1);
    }
}

static void test_invalid_arguments(void) {
    fixture_t fixture;
    init_fixture(&fixture, false, true);
    expect_run(&fixture, 0, true, 0);
    expect_run(&fixture, 30, true, 1);

    const uint32_t previous_reads = fixture.gpio.read_count;
    app_button_run(NULL);
    button_t unavailable_component = {0};
    app_button_run(&unavailable_component);
    app_button_run(&unavailable_component);
    CHECK(fixture.gpio.read_count == previous_reads);
    CHECK(screen_toggles == 1);

    // Invalid inputs preserve edge history and do not create another press.
    expect_run(&fixture, 40, true, 1);
    expect_run(&fixture, 50, false, 1);
    expect_run(&fixture, 80, false, 1);
    expect_run(&fixture, 90, true, 1);
    expect_run(&fixture, 120, true, 2);
}

static void test_tick_wrap(void) {
    fixture_t fixture;
    init_fixture(&fixture, false, true);
    expect_run(&fixture, UINT32_MAX - 15, true, 0);
    expect_run(&fixture, 13, true, 0);
    expect_run(&fixture, 14, true, 1);
    expect_run(&fixture, 15, true, 1);
}

static void test_active_high(void) {
    fixture_t fixture;
    init_fixture(&fixture, true, false);
    expect_run(&fixture, 0, false, 0);
    expect_run(&fixture, 30, false, 0);
    expect_run(&fixture, 40, true, 0);
    expect_run(&fixture, 70, true, 1);
    expect_run(&fixture, 100, true, 1);
    expect_run(&fixture, 110, false, 1);
    expect_run(&fixture, 140, false, 1);
    expect_run(&fixture, 150, true, 1);
    expect_run(&fixture, 180, true, 2);
}

static void test_reinitialization(void) {
    fixture_t fixture;
    init_fixture(&fixture, false, true);
    expect_run(&fixture, 0, true, 0);
    expect_run(&fixture, 30, true, 1);
    fixture.gpio.status = HAL_STATUS_ERROR;
    expect_run(&fixture, 40, true, 1);
    const uint32_t previous_reads = fixture.gpio.read_count;
    app_button_init();
    CHECK(fixture.gpio.read_count == previous_reads);
    CHECK(screen_toggles == 1);
    fixture.gpio.status = HAL_STATUS_OK;
    expect_run(&fixture, 50, true, 2);
    expect_run(&fixture, 80, true, 2);
}

int main(int argc, char **argv) {
    static const struct {
        const char *name;
        void (*run)(void);
    } scenarios[] = {
        {"run_before_init", test_run_before_init},
        {"press_hold_release", test_press_hold_release},
        {"contact_bounce", test_contact_bounce},
        {"pressed_at_startup", test_pressed_at_startup},
        {"failure_and_recovery", test_failure_and_recovery},
        {"unobserved_release", test_unobserved_release},
        {"failure_during_debounce", test_failure_during_debounce},
        {"read_failures", test_read_failures},
        {"invalid_arguments", test_invalid_arguments},
        {"tick_wrap", test_tick_wrap},
        {"active_high", test_active_high},
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
