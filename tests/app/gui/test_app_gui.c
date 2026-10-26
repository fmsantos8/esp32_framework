/**
 * @file test_app_gui.c
 * @author Fabricio Santos (fmsantos8@ucs.br)
 * @brief GUI scenarios using real graphics with simulated acquisition, I2C, and time.
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "acquisition/app_acquisition.h"
#include "cmp_ssd1306_config.h"
#include "gui/app_gui.h"
#include "status_led/app_status_led.h"
#include "telemetry/app_telemetry.h"

#define CHECK(condition)                                                   \
    do {                                                                   \
        if (!(condition)) {                                                \
            fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #condition); \
            exit(EXIT_FAILURE);                                            \
        }                                                                  \
    } while (0)

/** @brief Display transport and visible panel RAM supplied by the host test. */
struct hal_i2c {
    hal_status_t status;                              /** Result returned by the selected write. */
    uint32_t fail_at;                                 /** Failing write number; zero applies status to every write. */
    uint32_t write_count;                             /** Number of attempted writes. */
    uint32_t controller_inits;                        /** Number of controller initialization commands. */
    uint32_t delay;                                   /** Time consumed by each write, in milliseconds. */
    uint32_t page;                                    /** Last selected display page. */
    uint8_t panel[SSD1306_FRAMEBUFFER_SIZE];           /** Simulated visible panel contents. */
};

/** @brief Display context and its external transport. */
typedef struct {
    struct hal_i2c bus; /** Simulated bus and panel. */
    ssd1306_t display;  /** Real display component initialized by composition. */
} fixture_t;

static uint32_t simulated_ticks;
static uint32_t clock_reads;
static uint32_t acquisition_reads;
static uint32_t led_updates;
static uint32_t telemetry_errors;
static int32_t telemetry_error_code;
static bool display_i2c_error;
static bool has_result;
static acquisition_t acquisition;

static uint32_t get_ticks(void) {
    ++clock_reads;
    return simulated_ticks;
}

bool app_acquisition_get_result(acquisition_t *result) {
    CHECK(result != NULL);
    ++acquisition_reads;
    if (!has_result) {
        return false;
    }
    *result = acquisition;
    return true;
}

void app_status_led_set_i2c_error(app_status_led_i2c_source_t source, bool active) {
    CHECK(source == APP_STATUS_LED_I2C_DISPLAY);
    ++led_updates;
    display_i2c_error = active;
}

bool app_telemetry_report_error(telemetry_source_t source, int32_t code) {
    CHECK(source == APP_TELEMETRY_SOURCE_DISPLAY && code != SSD1306_STATUS_OK);
    ++telemetry_errors;
    telemetry_error_code = code;
    return true;
}

hal_status_t hal_i2c_write(hal_i2c_t *i2c, uint8_t address, const uint8_t *buffer, uint32_t size) {
    CHECK(i2c != NULL && buffer != NULL && address == 0x3C);
    ++i2c->write_count;
    simulated_ticks += i2c->delay;
    const hal_status_t status = i2c->fail_at == 0 || i2c->fail_at == i2c->write_count ? i2c->status : HAL_STATUS_OK;
    if (size == 30 && buffer[0] == 0x00 && buffer[1] == 0xAE) {
        ++i2c->controller_inits;
    } else if (size == 2 && buffer[0] == 0x00 && buffer[1] == 0xAF) {
        // Controller initialization turns the panel on after clearing its RAM.
    } else if (size == 4 && buffer[0] == 0x00 && (buffer[1] & 0xF8U) == 0xB0) {
        CHECK(buffer[2] == 0 && buffer[3] == 0x10);
        if (status == HAL_STATUS_OK) {
            i2c->page = buffer[1] & 7U;
        }
    } else {
        CHECK(size == SSD1306_WIDTH + 1 && buffer[0] == 0x40);
        // A failed data write may already have changed part of the physical page.
        memcpy(i2c->panel + i2c->page * SSD1306_WIDTH, buffer + 1, status == HAL_STATUS_OK ? SSD1306_WIDTH : 7);
    }
    return status;
}

static void init_fixture(fixture_t *fixture, uint32_t ticks) {
    *fixture = (fixture_t){.bus.status = HAL_STATUS_OK};
    simulated_ticks = ticks;
    clock_reads = 0;
    acquisition_reads = 0;
    led_updates = 0;
    telemetry_errors = 0;
    display_i2c_error = false;
    has_result = true;
    acquisition = (acquisition_t){
        .status = SHT31D_STATUS_OK,
        .sample = {.temperature = 24800, .humidity = 58300},
        .has_sample = true,
    };
    const ssd1306_config_t config = {.i2c = &fixture->bus, .address = 0x3C};
    CHECK(cmp_ssd1306_init(&fixture->display, &config) == SSD1306_STATUS_OK);
    CHECK(fixture->bus.controller_inits == 1);
    fixture->bus.write_count = 0;
    CHECK(app_gui_init(get_ticks));
    CHECK(fixture->bus.write_count == 0 && acquisition_reads == 0 && led_updates == 0);
    CHECK(telemetry_errors == 0);
}

static void run(fixture_t *fixture, uint32_t ticks) {
    simulated_ticks = ticks;
    const uint32_t previous_writes = fixture->bus.write_count;
    const uint32_t previous_inits = fixture->bus.controller_inits;
    app_gui_run(&fixture->display);
    CHECK(fixture->bus.write_count - previous_writes <= 2);
    CHECK(fixture->bus.controller_inits == previous_inits);
}

static void complete_frame(fixture_t *fixture, uint32_t ticks) {
    for (uint32_t page = 0; page < SSD1306_HEIGHT / 8U; ++page) {
        run(fixture, ticks);
    }
    CHECK(fixture->display.dirty_pages == 0);
    CHECK(memcmp(fixture->bus.panel, fixture->display.framebuffer, sizeof(fixture->bus.panel)) == 0);
}

static void no_transfer(fixture_t *fixture, uint32_t ticks) {
    const uint32_t previous_writes = fixture->bus.write_count;
    const uint32_t previous_updates = led_updates;
    run(fixture, ticks);
    CHECK(fixture->bus.write_count == previous_writes && led_updates == previous_updates);
}

static uint32_t frame_hash(const fixture_t *fixture) {
    uint32_t hash = 2166136261U;
    for (uint32_t index = 0; index < SSD1306_FRAMEBUFFER_SIZE; ++index) {
        hash = (hash ^ fixture->display.framebuffer[index]) * 16777619U;
    }
    return hash;
}

static void test_run_before_init(void) {
    ssd1306_t display = {0};
    CHECK(!app_gui_init(NULL));
    app_gui_toggle_screen();
    app_gui_run(NULL);
    app_gui_run(&display);
    CHECK(clock_reads == 0 && acquisition_reads == 0 && led_updates == 0);
}

static void test_screens_and_layout(void) {
    fixture_t fixture;
    init_fixture(&fixture, 3723000);
    complete_frame(&fixture, 3723000);
    // Reference hashes were captured from the legacy renderer before refactoring.
    CHECK(frame_hash(&fixture) == 0x93CA7465U);
    app_gui_toggle_screen();
    complete_frame(&fixture, 3723000);
    CHECK(frame_hash(&fixture) == 0x6C7ACC69U);
    app_gui_toggle_screen();
    complete_frame(&fixture, 3723000);
    CHECK(frame_hash(&fixture) == 0x93CA7465U);
}

static void test_startup(void) {
    fixture_t fixture;
    init_fixture(&fixture, 0);
    has_result = false;
    complete_frame(&fixture, 0);
    CHECK(frame_hash(&fixture) == 0x5D6149BDU);
    app_gui_toggle_screen();
    complete_frame(&fixture, 0);
    CHECK(frame_hash(&fixture) == 0x9A7F7C1AU);
}

static void test_acquisition_errors(void) {
    static const sht31d_status_t statuses[] = {SHT31D_STATUS_CRC_ERROR, SHT31D_STATUS_TIMEOUT, SHT31D_STATUS_COMMUNICATION_ERROR};
    for (uint32_t i = 0; i < sizeof(statuses) / sizeof(statuses[0]); ++i) {
        fixture_t fixture;
        init_fixture(&fixture, 3723000);
        complete_frame(&fixture, 3723000);
        acquisition.status = statuses[i];
        complete_frame(&fixture, 3723000);
        CHECK(frame_hash(&fixture) == 0x9C43137FU);
        CHECK(!display_i2c_error);
        acquisition.status = SHT31D_STATUS_OK;
        complete_frame(&fixture, 3723000);
        CHECK(frame_hash(&fixture) == 0x93CA7465U);
        acquisition.status = SHT31D_STATUS_NOT_INITIALIZED;
        acquisition.has_sample = false;
        complete_frame(&fixture, 3723000);
        CHECK(frame_hash(&fixture) == 0x9F98A5C7U);
    }
}

static void test_incremental_frame(void) {
    fixture_t fixture;
    init_fixture(&fixture, 0);
    run(&fixture, 0);
    CHECK(fixture.bus.write_count == 2 && acquisition_reads == 1);
    uint8_t original[SSD1306_FRAMEBUFFER_SIZE];
    memcpy(original, fixture.display.framebuffer, sizeof(original));
    acquisition.status = SHT31D_STATUS_TIMEOUT;
    acquisition.sample.temperature = 130000;
    app_gui_toggle_screen();
    for (uint32_t page = 1; page < SSD1306_HEIGHT / 8U; ++page) {
        run(&fixture, 1000);
        CHECK(memcmp(original, fixture.display.framebuffer, sizeof(original)) == 0);
    }
    CHECK(acquisition_reads == 1);
    CHECK(memcmp(original, fixture.bus.panel, sizeof(original)) == 0);
    run(&fixture, 1000);
    CHECK(acquisition_reads == 2);
    CHECK(memcmp(original, fixture.display.framebuffer, sizeof(original)) != 0);
    complete_frame(&fixture, 1000);
}

static void test_idle_updates(void) {
    fixture_t fixture;
    init_fixture(&fixture, 0);
    complete_frame(&fixture, 0);
    no_transfer(&fixture, 1000);
    acquisition.acquired_at = 1000;
    no_transfer(&fixture, 1000);
    app_gui_toggle_screen();
    complete_frame(&fixture, 1000);
    const uint32_t previous_hash = frame_hash(&fixture);
    no_transfer(&fixture, 1999);
    run(&fixture, 2000);
    CHECK(frame_hash(&fixture) != previous_hash);
    complete_frame(&fixture, 2000);
}

static void transfer_failure(uint32_t failing_write, hal_status_t status) {
    fixture_t fixture;
    init_fixture(&fixture, 0);
    fixture.bus.status = status;
    fixture.bus.fail_at = failing_write;
    fixture.bus.delay = 25;
    run(&fixture, 0);
    CHECK(fixture.bus.write_count == failing_write && display_i2c_error);
    CHECK(telemetry_errors == 1);
    CHECK(telemetry_error_code == (status == HAL_STATUS_TIMEOUT ? SSD1306_STATUS_TIMEOUT : SSD1306_STATUS_COMMUNICATION_ERROR));
    CHECK(fixture.display.dirty_pages != 0);
    const uint32_t failed_at = simulated_ticks;
    no_transfer(&fixture, failed_at + 999U);
    CHECK(telemetry_errors == 1);
    fixture.bus.status = HAL_STATUS_OK;
    fixture.bus.delay = 0;
    run(&fixture, failed_at + 1000U);
    CHECK(fixture.bus.write_count == failing_write + 2 && fixture.bus.page == 0);
    CHECK(!display_i2c_error);
    CHECK(telemetry_errors == 1);
    complete_frame(&fixture, failed_at + 1000U);
    CHECK(frame_hash(&fixture) == 0x93CA7465U);
}

static void test_command_failure(void) {
    transfer_failure(1, HAL_STATUS_ERROR);
    transfer_failure(1, HAL_STATUS_TIMEOUT);
}

static void test_data_failure(void) {
    transfer_failure(2, HAL_STATUS_ERROR);
    transfer_failure(2, HAL_STATUS_TIMEOUT);
}

static void test_repeated_i2c_errors(void) {
    fixture_t fixture;
    init_fixture(&fixture, 0);
    fixture.bus.status = HAL_STATUS_ERROR;
    run(&fixture, 0);
    CHECK(telemetry_errors == 1);
    no_transfer(&fixture, 999);
    CHECK(telemetry_errors == 1);
    run(&fixture, 1000);
    CHECK(telemetry_errors == 2 && display_i2c_error);
    no_transfer(&fixture, 1999);
    CHECK(telemetry_errors == 2);
    fixture.bus.status = HAL_STATUS_OK;
    complete_frame(&fixture, 2000);
    CHECK(telemetry_errors == 2 && !display_i2c_error);
}

static void test_retry_tick_wrap(void) {
    fixture_t fixture;
    const uint32_t start = UINT32_MAX - 500U;
    init_fixture(&fixture, start);
    fixture.bus.status = HAL_STATUS_ERROR;
    run(&fixture, start);
    CHECK(display_i2c_error);
    no_transfer(&fixture, start + 999U);
    fixture.bus.status = HAL_STATUS_OK;
    run(&fixture, start + 1000U);
    CHECK(!display_i2c_error);
    complete_frame(&fixture, start + 1000U);
}

static void test_uptime_wrap(void) {
    fixture_t fixture;
    init_fixture(&fixture, UINT32_MAX - 499U);
    app_gui_toggle_screen();
    complete_frame(&fixture, UINT32_MAX - 499U);
    complete_frame(&fixture, 1000);
    // 2^32 + 1000 milliseconds: the day count must survive the counter wrap.
    CHECK(frame_hash(&fixture) == 0x4A695352U);
}

static void test_invalid_arguments(void) {
    fixture_t fixture;
    init_fixture(&fixture, 3723000);
    app_gui_toggle_screen();
    complete_frame(&fixture, 3723000);
    const uint32_t previous_clock_reads = clock_reads;
    const uint32_t previous_writes = fixture.bus.write_count;
    CHECK(!app_gui_init(NULL));
    app_gui_run(NULL);
    CHECK(clock_reads == previous_clock_reads && fixture.bus.write_count == previous_writes);
    no_transfer(&fixture, 3723000);
    CHECK(frame_hash(&fixture) == 0x6C7ACC69U);
    app_gui_toggle_screen();
    app_gui_toggle_screen();
    no_transfer(&fixture, 3723000);
}

static void test_invalid_sample(void) {
    static const sht31d_sample_t invalid[] = {
        {.temperature = -45001, .humidity = 58300},
        {.temperature = 130001, .humidity = 58300},
        {.temperature = INT32_MIN, .humidity = 58300},
        {.temperature = 24800, .humidity = UINT32_MAX},
    };
    for (uint32_t i = 0; i < sizeof(invalid) / sizeof(invalid[0]); ++i) {
        fixture_t fixture;
        init_fixture(&fixture, 0);
        complete_frame(&fixture, 0);
        uint8_t previous_panel[SSD1306_FRAMEBUFFER_SIZE];
        memcpy(previous_panel, fixture.bus.panel, sizeof(previous_panel));
        acquisition.sample = invalid[i];
        no_transfer(&fixture, 0);
        CHECK(memcmp(previous_panel, fixture.bus.panel, sizeof(previous_panel)) == 0);
        CHECK(!display_i2c_error);
        acquisition.sample = (sht31d_sample_t){.temperature = 24800, .humidity = 58300};
        no_transfer(&fixture, 999);
        complete_frame(&fixture, 1000);
        CHECK(frame_hash(&fixture) == 0x93CA7465U);
    }
}

static void test_component_unavailable(void) {
    fixture_t fixture;
    init_fixture(&fixture, 0);
    fixture.display = (ssd1306_t){0};
    no_transfer(&fixture, 0);
    CHECK(!display_i2c_error);
    const ssd1306_config_t config = {.i2c = &fixture.bus, .address = 0x3C};
    CHECK(cmp_ssd1306_init(&fixture.display, &config) == SSD1306_STATUS_OK);
    no_transfer(&fixture, 999);
    complete_frame(&fixture, 1000);
    CHECK(frame_hash(&fixture) == 0x93CA7465U);
}

static void test_reinitialization(void) {
    fixture_t fixture;
    init_fixture(&fixture, 3723000);
    app_gui_toggle_screen();
    complete_frame(&fixture, 3723000);
    const uint32_t previous_writes = fixture.bus.write_count;
    CHECK(app_gui_init(get_ticks));
    CHECK(fixture.bus.write_count == previous_writes);
    complete_frame(&fixture, 3723000);
    CHECK(frame_hash(&fixture) == 0x93CA7465U);
}

int main(int argc, char **argv) {
    static const struct {
        const char *name;
        void (*run)(void);
    } scenarios[] = {
        {"run_before_init", test_run_before_init},
        {"screens_and_layout", test_screens_and_layout},
        {"startup", test_startup},
        {"acquisition_errors", test_acquisition_errors},
        {"incremental_frame", test_incremental_frame},
        {"idle_updates", test_idle_updates},
        {"command_failure", test_command_failure},
        {"data_failure", test_data_failure},
        {"repeated_i2c_errors", test_repeated_i2c_errors},
        {"retry_tick_wrap", test_retry_tick_wrap},
        {"uptime_wrap", test_uptime_wrap},
        {"invalid_arguments", test_invalid_arguments},
        {"invalid_sample", test_invalid_sample},
        {"component_unavailable", test_component_unavailable},
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
