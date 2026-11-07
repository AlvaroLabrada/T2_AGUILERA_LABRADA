#include "bme280_port.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "pines.h"
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "main";
static uint32_t contador_carga1 = 0;
static uint32_t contador_carga2 = 0;

static void tarea_carga_1(void *arg) {

  // Rafaga de calculo de 20ms de CPU activa

  while (1) {
    int64_t inicio = esp_timer_get_time();
    while ((esp_timer_get_time() - inicio) < 20000) {
      // Espera bloqueada hasta completar los 20 ms
    }
    contador_carga1++;
    // 2. Espera BLOQUEADA de 27 ms (cede el procesador)
    vTaskDelay(pdMS_TO_TICKS(27));
  }
}
//
static void tarea_carga_2(void *arg) {

  // Rafaga de calculo de 20ms de CPU activa

  while (1) {
    int64_t inicio = esp_timer_get_time();
    while ((esp_timer_get_time() - inicio) < 20000) {
      // Espera bloqueada hasta completar los 20 ms
    }
    contador_carga2++;
    // 2. Espera BLOQUEADA de 27 ms (cede el procesador)
    vTaskDelay(pdMS_TO_TICKS(27));
  }
}

static void tarea_informe(void *arg) {
  while (1) {
    vTaskDelay(pdMS_TO_TICKS(1000));
    ESP_LOGI(TAG, "Contador carga 1: %d", contador_carga1);
    ESP_LOGI(TAG, "Contador carga 2: %d", contador_carga2);
  }
}

void app_main(void) {
  ESP_LOGI(TAG, "Arrancando Taller 2 - Sesión de preparación");

  /* 1. Inicialización del sensor BME280 con los pines del bus I2C */
  ESP_LOGI(TAG, "Inicializando sensor BME280 (SDA=%d, SCL=%d)...", PIN_I2C_SDA,
           PIN_I2C_SCL);
  esp_err_t err =
      bme280_port_iniciar(PIN_I2C_SDA, PIN_I2C_SCL, BME280_I2C_ADDR);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Error al inicializar el sensor BME280: %s",
             esp_err_to_name(err));
    return;
  }

  xTaskCreatePinnedToCore(tarea_carga_1, "Tarea1", 3072, NULL, 2, NULL, 0);
  xTaskCreatePinnedToCore(tarea_carga_2, "Tarea2", 3072, NULL, 2, NULL, 0);
  xTaskCreatePinnedToCore(tarea_informe, "Informe", 3072, NULL, 1, NULL, 1);

  /* 2. Realizar una lectura de prueba y mostrarla en el monitor
  bme280_datos_t datos;
  err = bme280_port_leer(&datos);
  if (err == ESP_OK) {
    ESP_LOGI(TAG, "--- Lectura inicial del sensor BME280 ---");
    ESP_LOGI(TAG, "  Temperatura : %.2f °C", datos.temperatura);
    ESP_LOGI(TAG, "  Presion     : %.2f Pa (%.2f hPa)", datos.presion,
             datos.presion / 100.0f);
    ESP_LOGI(TAG, "  Humedad     : %.2f %%", datos.humedad);
    ESP_LOGI(TAG, "Sensor listo para las practicas del Taller 2.");
  } else {
    ESP_LOGE(TAG, "Error al obtener la lectura del sensor: %s",
             esp_err_to_name(err));
  } */
}
