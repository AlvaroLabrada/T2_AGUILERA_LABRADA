idf_component_register(SRCS "Ejercicio2.c" "main.c" "Ejercicio1.c"
                       INCLUDE_DIRS "."
                       REQUIRES led pulsador bme280 consola esp_driver_gpio)
