idf_component_register(
    SRCS
        "gpio/hal_gpio.c"
        "timer/hal_timer.c"
        "uart/hal_uart.c"
    INCLUDE_DIRS "." "../../interface"
    REQUIRES esp_driver_gpio esp_driver_i2c esp_driver_gptimer esp_driver_uart
)
