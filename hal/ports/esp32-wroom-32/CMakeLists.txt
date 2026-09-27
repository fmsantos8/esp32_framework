idf_component_register(
    SRCS
        "gpio/hal_gpio.c"
    INCLUDE_DIRS "." "../../interface"
    REQUIRES esp_driver_gpio esp_driver_i2c esp_driver_gptimer esp_driver_uart
)
