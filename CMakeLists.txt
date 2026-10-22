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
        "app" "components/button" "${HAL_ESP32_INTERFACE_DIR}"
    )
    set_target_properties(app_button_tests PROPERTIES
        C_STANDARD 11
        C_STANDARD_REQUIRED YES
        C_EXTENSIONS NO
    )
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(app_button_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
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
        button_binding
        optional_callbacks
        reinitialization
    )
        add_test(NAME "app_button.${scenario}" COMMAND app_button_tests "${scenario}")
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
            "components/button/cmp_button.c"
            "components/led/cmp_led.c"
            "components/telemetry/cmp_telemetry.c"
            "hal/ports/esp32-wroom-32/hal_esp32.c"
            "hal/ports/esp32-wroom-32/system/hal_system.c"
            "hal/ports/esp32-wroom-32/gpio/hal_gpio.c"
            "hal/ports/esp32-wroom-32/i2c/hal_i2c.c"
            "hal/ports/esp32-wroom-32/uart/hal_uart.c"
            "hal/ports/esp32-wroom-32/timer/hal_timer.c"
            "${HAL_ESP32_MEMORY_DIR}/hal_memory.c"
        INCLUDE_DIRS "app" "bsp" "components/button" "components/led" "components/telemetry" "${HAL_ESP32_INTERFACE_DIR}"
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
