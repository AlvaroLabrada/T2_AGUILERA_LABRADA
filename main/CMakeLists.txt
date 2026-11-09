idf_component_register(SRCS "main.c" "Ejercicio1.c"
                       INCLUDE_DIRS "."
                       REQUIRES led pulsador bme280 consola esp_driver_gpio)
