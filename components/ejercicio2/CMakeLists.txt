idf_component_register(SRCS "Ejercicio2.c"
                       INCLUDE_DIRS "include"
                       PRIV_INCLUDE_DIRS "${PROJECT_DIR}/main" 
                       PRIV_REQUIRES freertos esp_timer bme280 consola ejercicio1)

                       