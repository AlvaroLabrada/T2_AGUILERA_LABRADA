idf_component_register(SRCS "Ejercicio3.c"
                       INCLUDE_DIRS "include"
                       PRIV_INCLUDE_DIRS "${PROJECT_DIR}/main"
                       PRIV_REQUIRES freertos esp_timer esp_driver_gpio esp_driver_ledc pulsador consola)