idf_component_register(
    SRCS
        "port_resources.c"
        "port_info.c"
        "gpio/hal_gpio.c"
        "i2c/hal_i2c.c"
        "timer/hal_timer.c"
        "uart/hal_uart.c"
    INCLUDE_DIRS "../../interface"
    PRIV_INCLUDE_DIRS "."
    REQUIRES memory
    PRIV_REQUIRES esp_driver_gpio esp_driver_i2c esp_driver_uart esp_timer
)
target_compile_features(${COMPONENT_LIB} PRIVATE c_std_11)
