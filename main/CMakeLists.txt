idf_component_register(SRCS "Ejercicio4.c" "Ejercicio2.c" "main.c" "Ejercicio1.c" "Ejercicio3.c"
                       INCLUDE_DIRS "."
                       REQUIRES led pulsador bme280 consola esp_driver_gpio esp_driver_ledc)
