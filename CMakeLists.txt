cmake_minimum_required(VERSION 3.22)

set(HAL_ESP32_INTERFACE_DIR "${CMAKE_CURRENT_LIST_DIR}/hal/interface" CACHE PATH "Portable HAL interface directory")
set(HAL_ESP32_MEMORY_DIR "${CMAKE_CURRENT_LIST_DIR}/hal/memory" CACHE PATH "HAL arena directory")

option(FRAMEWORK_BUILD_HOST_TESTS "Build isolated application tests without ESP-IDF" OFF)
if(FRAMEWORK_BUILD_HOST_TESTS)
    project(esp32_framework_host_tests LANGUAGES C)
    enable_testing()

    add_executable(app_button_tests
        "tests/app/button/test_app_button.c"
        "app/button/app_button.c"
        "components/button/cmp_button.c"
    )
    target_include_directories(app_button_tests PRIVATE
        "app" "components/button" "components/ssd1306" "${HAL_ESP32_INTERFACE_DIR}"
    )
    add_executable(app_status_led_tests
        "tests/app/status_led/test_app_status_led.c"
        "app/status_led/app_status_led.c"
        "components/led/cmp_led.c"
    )
    target_include_directories(app_status_led_tests PRIVATE
        "app" "components/led" "${HAL_ESP32_INTERFACE_DIR}"
    )
    add_executable(app_acquisition_tests
        "tests/app/acquisition/test_app_acquisition.c"
        "app/acquisition/app_acquisition.c"
        "components/sht31d/cmp_sht31d.c"
    )
    target_include_directories(app_acquisition_tests PRIVATE
        "app" "components/sht31d" "components/led" "components/telemetry" "${HAL_ESP32_INTERFACE_DIR}"
    )
    add_executable(app_gui_tests
        "tests/app/gui/test_app_gui.c"
        "app/gui/app_gui.c"
        "app/gui/render.c"
        "app/gui/bitmaps.c"
        "components/ssd1306/cmp_ssd1306.c"
        "components/ssd1306/cmp_ssd1306_fonts.c"
    )
    add_executable(app_gui_preview
        "scripts/interface_preview/host.c"
        "app/gui/app_gui.c"
        "app/gui/render.c"
        "app/gui/bitmaps.c"
        "components/ssd1306/cmp_ssd1306.c"
        "components/ssd1306/cmp_ssd1306_fonts.c"
    )
    foreach(gui_target app_gui_tests app_gui_preview)
        target_include_directories(${gui_target} PRIVATE
            "app" "components/ssd1306" "components/sht31d" "components/led" "components/telemetry" "${HAL_ESP32_INTERFACE_DIR}"
        )
    endforeach()
    set_target_properties(app_gui_preview PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_BINARY_DIR}/bin"
    )
    add_executable(app_telemetry_tests
        "tests/app/telemetry/test_app_telemetry.c"
        "app/telemetry/app_telemetry.c"
        "components/telemetry/cmp_telemetry.c"
    )
    target_include_directories(app_telemetry_tests PRIVATE
        "app" "components/telemetry" "${HAL_ESP32_INTERFACE_DIR}"
    )
    foreach(test_target app_button_tests app_status_led_tests app_acquisition_tests app_gui_tests app_gui_preview app_telemetry_tests)
        set_target_properties(${test_target} PROPERTIES
            C_STANDARD 11
            C_STANDARD_REQUIRED YES
            C_EXTENSIONS NO
        )
        if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
            target_compile_options(${test_target} PRIVATE -Wall -Wextra -Wpedantic -Werror)
        endif()
    endforeach()
    foreach(scenario
        run_before_init
        press_hold_release
        contact_bounce
        pressed_at_startup
        failure_and_recovery
        unobserved_release
        failure_during_debounce
        read_failures
        invalid_arguments
        tick_wrap
        active_high
        reinitialization
    )
        add_test(NAME "app_button.${scenario}" COMMAND app_button_tests "${scenario}")
    endforeach()

    foreach(scenario
        run_before_init
        startup_normal
        blink_recovery
        blink_restart
        independent_i2c_errors
        startup_i2c_error
        repeated_notifications
        tick_wrap_and_late_runs
        write_results_ignored
        component_unavailable
        invalid_arguments
        reinitialization
    )
        add_test(NAME "app_status_led.${scenario}" COMMAND app_status_led_tests "${scenario}")
    endforeach()

    foreach(scenario
        run_before_init
        periodic_samples
        uninitialized_sensor
        start_failure
        read_failure_retains_sample
        crc_failures
        i2c_recovery_with_crc
        invalid_arguments
        driver_errors
        conversion_from_return
        tick_wrap
        late_runs
        result_readers
        reinitialization
        pending_reinitialization
    )
        add_test(NAME "app_acquisition.${scenario}" COMMAND app_acquisition_tests "${scenario}")
    endforeach()
    foreach(scenario
        run_before_init
        screens_and_layout
        startup
        acquisition_errors
        incremental_frame
        idle_updates
        command_failure
        data_failure
        repeated_i2c_errors
        retry_tick_wrap
        uptime_wrap
        invalid_arguments
        invalid_sample
        component_unavailable
        reinitialization
    )
        add_test(NAME "app_gui.${scenario}" COMMAND app_gui_tests "${scenario}")
    endforeach()
    foreach(scenario
        run_before_init
        fifo_and_copy
        duplicate_errors
        value_ranges
        queue_overflow
        queue_wrap
        zero_byte_retry
        partial_write_discard
        permanent_failures
        component_unavailable
        invalid_arguments
        reinitialization
        tick_wrap
        repeated_uart_failures
        queue_full_retry
    )
        add_test(NAME "app_telemetry.${scenario}" COMMAND app_telemetry_tests "${scenario}")
    endforeach()
    return()
endif()

# ESP-IDF reads this file again to register the firmware as one component.
if(DEFINED COMPONENT_NAME AND "${COMPONENT_DIR}" STREQUAL "${CMAKE_CURRENT_LIST_DIR}")
    if(NOT IDF_VERSION_MAJOR EQUAL 6 OR NOT IDF_VERSION_MINOR EQUAL 1)
        message(FATAL_ERROR "esp32_framework requires ESP-IDF 6.1.x")
    endif()

    idf_component_register(
        SRCS
            "main/main.c"
            "bsp/esp32_devkit_v1/bsp.c"
            "app/button/app_button.c"
            "app/status_led/app_status_led.c"
            "app/acquisition/app_acquisition.c"
            "app/telemetry/app_telemetry.c"
            "app/gui/app_gui.c"
            "app/gui/render.c"
            "app/gui/bitmaps.c"
            "components/button/cmp_button.c"
            "components/led/cmp_led.c"
            "components/sht31d/cmp_sht31d.c"
            "components/ssd1306/cmp_ssd1306.c"
            "components/ssd1306/cmp_ssd1306_fonts.c"
            "components/telemetry/cmp_telemetry.c"
            "hal/ports/esp32-wroom-32/hal_esp32.c"
            "hal/ports/esp32-wroom-32/system/hal_system.c"
            "hal/ports/esp32-wroom-32/gpio/hal_gpio.c"
            "hal/ports/esp32-wroom-32/i2c/hal_i2c.c"
            "hal/ports/esp32-wroom-32/uart/hal_uart.c"
            "hal/ports/esp32-wroom-32/timer/hal_timer.c"
            "${HAL_ESP32_MEMORY_DIR}/hal_memory.c"
        INCLUDE_DIRS "app" "bsp" "components/button" "components/led" "components/sht31d" "components/ssd1306" "components/telemetry" "${HAL_ESP32_INTERFACE_DIR}"
        PRIV_INCLUDE_DIRS "hal/ports/esp32-wroom-32" "${HAL_ESP32_MEMORY_DIR}"
        PRIV_REQUIRES
            esp_driver_gpio esp_driver_i2c esp_driver_uart esp_driver_gptimer
            esp_hw_support esp_pm freertos esp_common
        REQUIRED_IDF_TARGETS esp32
    )

    target_compile_features(${COMPONENT_LIB} PUBLIC c_std_11)
    return()
endif()

set(EXTRA_COMPONENT_DIRS "${CMAKE_CURRENT_LIST_DIR}")
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(esp32_framework)
