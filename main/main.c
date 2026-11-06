#include <stdio.h>
#include "esp_log.h"
#include "esp_err.h"
#include "pines.h"
#include "bme280_port.h"

static const char *TAG = "main";

void app_main(void)
{
    ESP_LOGI(TAG, "Arrancando Taller 2 - Sesión de preparación");

    /* 1. Inicialización del sensor BME280 con los pines del bus I2C */
    ESP_LOGI(TAG, "Inicializando sensor BME280 (SDA=%d, SCL=%d)...", PIN_I2C_SDA, PIN_I2C_SCL);
    esp_err_t err = bme280_port_iniciar(PIN_I2C_SDA, PIN_I2C_SCL, BME280_I2C_ADDR);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error al inicializar el sensor BME280: %s", esp_err_to_name(err));
        return;
    }

    /* 2. Realizar una lectura de prueba y mostrarla en el monitor */
    bme280_datos_t datos;
    err = bme280_port_leer(&datos);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "--- Lectura inicial del sensor BME280 ---");
        ESP_LOGI(TAG, "  Temperatura : %.2f °C", datos.temperatura);
        ESP_LOGI(TAG, "  Presion     : %.2f Pa (%.2f hPa)", datos.presion, datos.presion / 100.0f);
        ESP_LOGI(TAG, "  Humedad     : %.2f %%", datos.humedad);
        ESP_LOGI(TAG, "Sensor listo para las practicas del Taller 2.");
    } else {
        ESP_LOGE(TAG, "Error al obtener la lectura del sensor: %s", esp_err_to_name(err));
    }
}
