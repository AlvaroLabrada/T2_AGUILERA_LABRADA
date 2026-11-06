/* bme280_port.h - Interfaz del sensor BME280 para ESP32-S3 (ESP-IDF)
 * adaptada sobre la API oficial de Bosch Sensortec.
 */
#pragma once

#include <stdbool.h>
#include "esp_err.h"
#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float temperatura; /* Grados Celsius */
    float presion;     /* Pascales (Pa) */
    float humedad;     /* Humedad relativa (%) */
} bme280_datos_t;

/**
 * @brief Inicializa el bus I2C y el sensor BME280.
 *
 * Configura el controlador maestro I2C en los pines indicados, detecta el
 * sensor en la dirección dada, inicializa la API Bosch y ajusta los
 * sobremuestreos a 1X en modo forzado.
 *
 * @param pin_sda GPIO para la línea de datos I2C (SDA).
 * @param pin_scl GPIO para la línea de reloj I2C (SCL).
 * @param direccion_i2c Dirección I2C del sensor (típicamente 0x76 o 0x77).
 * @return ESP_OK si la inicialización fue correcta.
 */
esp_err_t bme280_port_iniciar(gpio_num_t pin_sda, gpio_num_t pin_scl, uint8_t direccion_i2c);

/**
 * @brief Realiza una lectura bajo demanda en modo forzado.
 *
 * Dispara una conversión, espera el tiempo necesario calculado por la API
 * y recupera las magnitudes compensadas (temperatura, presión y humedad).
 *
 * @param[out] datos Estructura donde se guardan los valores medidos.
 * @return ESP_OK si la lectura tuvo éxito.
 */
esp_err_t bme280_port_leer(bme280_datos_t *datos);

#ifdef __cplusplus
}
#endif

