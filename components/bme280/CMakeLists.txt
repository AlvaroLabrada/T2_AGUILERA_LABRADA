idf_component_register(
    SRCS "bme280.c" "bme280_port.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_driver_i2c esp_driver_gpio esp_rom freertos
)
