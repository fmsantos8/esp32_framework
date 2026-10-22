/**
 * @file test_app_button.c
 * @author Fabricio Santos (fmsantos8@ucs.br)
 * @brief Callback task scenarios using the real component and simulated GPIO and time.
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "button/app_button.h"
#include "cmp_button_config.h"

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

/** @brief Receiver spy for press notifications. */
typedef struct {
    uint32_t count; /** Number of received presses. */
} press_receiver_t;

/** @brief Component and receiver storage for one simulated input. */
typedef struct {
    struct hal_gpio gpio;   /** Simulated GPIO. */
    button_t component;     /** Real button component. */
    bool active_level;      /** Component polarity. */
    press_receiver_t press; /** Separate press receiver context. */
} fixture_t;

static uint32_t simulated_ticks;
static uint32_t null_context_press_count;

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

static void on_press(void *context) {
    press_receiver_t *receiver = context;
    CHECK(receiver != NULL);
    ++receiver->count;
}

static void on_press_without_context(void *context) {
    CHECK(context == NULL);
    ++null_context_press_count;
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
    const uint32_t previous_reads = fixture->gpio.read_count;
    CHECK(app_button_init(&fixture->component, on_press, &fixture->press));
    CHECK(fixture->gpio.read_count == previous_reads);
    CHECK(fixture->press.count == 0);
}

static void expect_run(fixture_t *fixture, uint32_t ticks, bool pressed, bool press_event) {
    simulated_ticks = ticks;
    fixture->gpio.level = pressed ? fixture->active_level : !fixture->active_level;
    const uint32_t previous_reads = fixture->gpio.read_count;
    const uint32_t previous_presses = fixture->press.count;
    app_button_run(&fixture->component);
    CHECK(fixture->gpio.read_count == previous_reads + 1);
    CHECK(fixture->press.count == previous_presses + (press_event ? 1U : 0U));
}

static void test_run_before_init(void) {
    fixture_t fixture;
    init_component(&fixture, false, true);
    const uint32_t previous_reads = fixture.gpio.read_count;
    CHECK(!app_button_init(NULL, on_press, &fixture.press));
    app_button_run(NULL);
    app_button_run(&fixture.component);
    CHECK(fixture.gpio.read_count == previous_reads);
    CHECK(fixture.press.count == 0);
}

static void test_press_hold_release(void) {
    fixture_t fixture;
    init_fixture(&fixture, false, false);
    expect_run(&fixture, 0, false, false);
    expect_run(&fixture, 30, false, false);
    expect_run(&fixture, 40, true, false);
    expect_run(&fixture, 69, true, false);
    expect_run(&fixture, 70, true, true);
    expect_run(&fixture, 1000, true, false);
    expect_run(&fixture, 1010, false, false);
    expect_run(&fixture, 1040, false, false);
    expect_run(&fixture, 1050, true, false);
    expect_run(&fixture, 1080, true, true);
    CHECK(fixture.press.count == 2);
}

static void test_contact_bounce(void) {
    fixture_t fixture;
    init_fixture(&fixture, false, false);
    expect_run(&fixture, 0, false, false);
    expect_run(&fixture, 30, false, false);
    expect_run(&fixture, 40, true, false);
    expect_run(&fixture, 45, false, false);
    expect_run(&fixture, 50, true, false);
    expect_run(&fixture, 60, false, false);
    expect_run(&fixture, 70, true, false);
    expect_run(&fixture, 99, true, false);
    expect_run(&fixture, 100, true, true);
    expect_run(&fixture, 110, false, false);
    expect_run(&fixture, 120, true, false);
    expect_run(&fixture, 130, false, false);
    expect_run(&fixture, 140, true, false);
    expect_run(&fixture, 170, false, false);
    expect_run(&fixture, 199, false, false);
    expect_run(&fixture, 200, false, false);
    expect_run(&fixture, 201, true, false);
    expect_run(&fixture, 231, true, true);
}

static void test_pressed_at_startup(void) {
    fixture_t fixture;
    init_fixture(&fixture, false, true);
    expect_run(&fixture, 0, true, false);
    expect_run(&fixture, 29, true, false);
    expect_run(&fixture, 30, true, true);
    expect_run(&fixture, 100, true, false);
}

static void test_failure_and_recovery(void) {
    fixture_t fixture;
    init_fixture(&fixture, false, true);
    expect_run(&fixture, 0, true, false);
    expect_run(&fixture, 30, true, true);
    fixture.gpio.status = HAL_STATUS_ERROR;
    expect_run(&fixture, 40, true, false);
    expect_run(&fixture, 41, true, false);

    fixture.gpio.status = HAL_STATUS_NOT_SUPPORTED;
    expect_run(&fixture, 42, true, false);
    expect_run(&fixture, 43, true, false);

    fixture.gpio.status = HAL_STATUS_OK;
    expect_run(&fixture, 50, true, false);
    expect_run(&fixture, 80, true, false);
    fixture.gpio.status = HAL_STATUS_NOT_SUPPORTED;
    expect_run(&fixture, 81, true, false);
    expect_run(&fixture, 82, true, false);
    fixture.gpio.status = HAL_STATUS_OK;
    expect_run(&fixture, 90, true, false);
    expect_run(&fixture, 100, false, false);
    expect_run(&fixture, 130, false, false);
    expect_run(&fixture, 140, true, false);
    expect_run(&fixture, 170, true, true);
}

static void test_unobserved_release(void) {
    fixture_t fixture;
    init_fixture(&fixture, false, true);
    expect_run(&fixture, 0, true, false);
    expect_run(&fixture, 30, true, true);
    fixture.gpio.status = HAL_STATUS_ERROR;
    expect_run(&fixture, 40, false, false);
    expect_run(&fixture, 70, false, false);
    expect_run(&fixture, 80, true, false);
    fixture.gpio.status = HAL_STATUS_OK;
    expect_run(&fixture, 90, true, false);
    expect_run(&fixture, 120, true, false);
    expect_run(&fixture, 130, false, false);
    expect_run(&fixture, 160, false, false);
    expect_run(&fixture, 170, true, false);
    expect_run(&fixture, 200, true, true);
}

static void test_failure_during_debounce(void) {
    fixture_t fixture;
    init_fixture(&fixture, false, false);
    expect_run(&fixture, 0, false, false);
    expect_run(&fixture, 30, false, false);
    expect_run(&fixture, 40, true, false);
    fixture.gpio.status = HAL_STATUS_ERROR;
    expect_run(&fixture, 50, true, false);
    fixture.gpio.status = HAL_STATUS_OK;
    expect_run(&fixture, 80, true, false);
    expect_run(&fixture, 109, true, false);
    expect_run(&fixture, 110, true, true);
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
        expect_run(&fixture, 0, true, false);
        expect_run(&fixture, 29, true, false);

        fixture.gpio.status = HAL_STATUS_OK;
        expect_run(&fixture, 30, true, false);
        expect_run(&fixture, 59, true, false);
        expect_run(&fixture, 60, true, true);
        expect_run(&fixture, 100, true, false);
    }
}

static void test_invalid_arguments(void) {
    fixture_t fixture;
    init_fixture(&fixture, false, true);
    expect_run(&fixture, 0, true, false);
    expect_run(&fixture, 30, true, true);

    const uint32_t previous_reads = fixture.gpio.read_count;
    CHECK(!app_button_init(NULL, on_press_without_context, NULL));
    app_button_run(NULL);
    CHECK(fixture.gpio.read_count == previous_reads);
    CHECK(fixture.press.count == 1);

    // A failed initialization preserves the previous receiver and edge history.
    expect_run(&fixture, 40, true, false);
    expect_run(&fixture, 50, false, false);
    expect_run(&fixture, 80, false, false);
    expect_run(&fixture, 90, true, false);
    expect_run(&fixture, 120, true, true);

    button_t unavailable_component = {0};
    const uint32_t reads_before_unavailable = fixture.gpio.read_count;
    CHECK(app_button_init(&unavailable_component, on_press, &fixture.press));
    app_button_run(&unavailable_component);
    app_button_run(&unavailable_component);
    CHECK(fixture.gpio.read_count == reads_before_unavailable);
    CHECK(fixture.press.count == 2);
}

static void test_tick_wrap(void) {
    fixture_t fixture;
    init_fixture(&fixture, false, true);
    expect_run(&fixture, UINT32_MAX - 15, true, false);
    expect_run(&fixture, 13, true, false);
    expect_run(&fixture, 14, true, true);
    expect_run(&fixture, 15, true, false);
}

static void test_button_binding(void) {
    fixture_t low;
    fixture_t high;
    init_fixture(&low, false, true);
    init_component(&high, true, true);
    expect_run(&low, 0, true, false);
    expect_run(&low, 30, true, true);

    const uint32_t high_reads = high.gpio.read_count;
    app_button_run(&high.component);
    CHECK(high.gpio.read_count == high_reads);
    CHECK(high.press.count == 0);
    expect_run(&low, 40, true, false);

    // Rebinding replaces the receiver and stops consuming the previous component.
    CHECK(app_button_init(&high.component, on_press, &high.press));
    CHECK(high.gpio.read_count == high_reads);
    const uint32_t low_reads = low.gpio.read_count;
    app_button_run(&low.component);
    CHECK(low.gpio.read_count == low_reads);
    CHECK(low.press.count == 1);
    expect_run(&high, 50, true, false);
    expect_run(&high, 80, true, true);
    expect_run(&high, 110, true, false);
    CHECK(high.press.count == 1);
}

static void test_optional_callbacks(void) {
    fixture_t fixture;
    init_fixture(&fixture, false, true);
    CHECK(app_button_init(&fixture.component, NULL, NULL));
    expect_run(&fixture, 0, true, false);
    expect_run(&fixture, 30, true, false);
    fixture.gpio.status = HAL_STATUS_ERROR;
    expect_run(&fixture, 40, true, false);

    init_fixture(&fixture, false, true);
    null_context_press_count = 0;
    CHECK(app_button_init(&fixture.component, on_press_without_context, NULL));
    expect_run(&fixture, 0, true, false);
    expect_run(&fixture, 30, true, false);
    expect_run(&fixture, 100, true, false);
    CHECK(null_context_press_count == 1);
}

static void test_reinitialization(void) {
    fixture_t fixture;
    init_fixture(&fixture, false, true);
    expect_run(&fixture, 0, true, false);
    expect_run(&fixture, 30, true, true);
    fixture.gpio.status = HAL_STATUS_ERROR;
    expect_run(&fixture, 40, true, false);
    const uint32_t previous_reads = fixture.gpio.read_count;
    CHECK(app_button_init(&fixture.component, on_press, &fixture.press));
    CHECK(fixture.gpio.read_count == previous_reads);
    CHECK(fixture.press.count == 1);
    fixture.gpio.status = HAL_STATUS_OK;
    expect_run(&fixture, 50, true, true);
    expect_run(&fixture, 80, true, false);
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
        {"button_binding", test_button_binding},
        {"optional_callbacks", test_optional_callbacks},
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
