set(HAL_ESP32_INTERFACE_DIR "${CMAKE_CURRENT_LIST_DIR}/hal/interface" CACHE PATH "Portable HAL interface directory")
set(HAL_ESP32_MEMORY_DIR "${CMAKE_CURRENT_LIST_DIR}/hal/memory" CACHE PATH "HAL arena directory")

if(NOT IDF_VERSION_MAJOR EQUAL 6 OR NOT IDF_VERSION_MINOR EQUAL 1)
    message(FATAL_ERROR "esp32_framework requires ESP-IDF 6.1.x")
endif()

idf_component_register(
    SRCS
        "main.c"
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
