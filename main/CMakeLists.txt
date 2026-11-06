idf_component_register(SRCS "ejercicio4.c" "Ejercicio2.c" "filtro.c" "main.c" "ejercicio1.c" "ejercicio3.c"
                    INCLUDE_DIRS "."
                    REQUIRES led esp_timer pulsador esp_driver_i2c bme280 esp_driver_uart)