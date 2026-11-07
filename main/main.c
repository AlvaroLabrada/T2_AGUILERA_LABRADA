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

  xTaskCreatePinnedToCore(tarea_carga_1, "Tarea1", 3072, NULL, 3, NULL, 0);
  xTaskCreatePinnedToCore(tarea_carga_2, "Tarea2", 3072, NULL, 2, NULL, 0);
  xTaskCreatePinnedToCore(tarea_informe, "Informe", 3072, NULL, 1, NULL, 1);
}
