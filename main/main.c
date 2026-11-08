#include "bme280_port.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "pines.h"
#include <stdio.h>
#include "consola.h"
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define PRIORIDAD_tarea_carga_1 3
#define PRIORIDAD_tarea_carga_2 2 
#define PRIORIDAD_tarea_informe 1 

static const char *TAG = "main";
static uint32_t contador_carga1 = 0;
static uint32_t contador_carga2 = 0;

typedef enum {
    MODO_PARADA = 0,
    MODO_TURNOS,
    MODO_INANICION,
    MODO_CORREGIDA
} modo_carga_t;

static volatile modo_carga_t s_modo = MODO_PARADA;

static void tarea_carga_1(void *arg) {
    while (1) {
        if (s_modo == MODO_PARADA) {
            // Solo duerme 27 ms y vuelve a empezar el bucle
            vTaskDelay(pdMS_TO_TICKS(27));
        } else {
            // Solo calcula si NO está en parada
            int64_t inicio = esp_timer_get_time();
            while ((esp_timer_get_time() - inicio) < 20000) {
            }
            contador_carga1++;

            // En inanición no duerme; en turnos y corregida duerme 27 ms
            if (s_modo == MODO_TURNOS) {
                vTaskPrioritySet(NULL, 2);
                vTaskDelay(pdMS_TO_TICKS(27));
            } else if (s_modo == MODO_INANICION) {
                vTaskPrioritySet(NULL, 3);
                // No duerme
            } else if (s_modo == MODO_CORREGIDA) {
                vTaskPrioritySet(NULL, 3);
                vTaskDelay(pdMS_TO_TICKS(27));
            }
        }
    }
}
//
static void tarea_carga_2(void *arg) {
    while (1) {
        if (s_modo == MODO_PARADA) {
            // Solo duerme 100 ms y nada más
            vTaskDelay(pdMS_TO_TICKS(27));
        } else {
            // Solo calcula si NO está en parada
            int64_t inicio = esp_timer_get_time();
            while ((esp_timer_get_time() - inicio) < 20000) {
            }
            contador_carga2++;
            
            // La tarea 2 siempre duerme sus 27 ms
            vTaskDelay(pdMS_TO_TICKS(27));
        }
    }
}

static void tarea_informe(void *arg) {
  while (1) {
    vTaskDelay(pdMS_TO_TICKS(1000));
    ESP_LOGI(TAG, "Contador carga 1: %d", contador_carga1);
    ESP_LOGI(TAG, "Contador carga 2: %d", contador_carga2);
  }
}

static void cmd_parada(char *args)    { s_modo = MODO_PARADA;    printf("Modo: parada\n"); }
static void cmd_turnos(char *args)    { s_modo = MODO_TURNOS;    printf("Modo: turnos\n"); }
static void cmd_inanicion(char *args) { s_modo = MODO_INANICION; printf("Modo: inanicion\n"); }
static void cmd_corregida(char *args) { s_modo = MODO_CORREGIDA; printf("Modo: corregida\n"); }

void app_main(void) {
    ESP_LOGI(TAG, "Arrancando Taller 2 - Sesión de preparación");

    xTaskCreatePinnedToCore(tarea_carga_1, "Tarea1", 3072, NULL, PRIORIDAD_tarea_carga_1, NULL, 0);
    xTaskCreatePinnedToCore(tarea_carga_2, "Tarea2", 3072, NULL, PRIORIDAD_tarea_carga_2, NULL, 0);
    xTaskCreatePinnedToCore(tarea_informe, "Informe", 3072, NULL, PRIORIDAD_tarea_informe, NULL, 1);

    // Registramos cada modo directamente como comando
    consola_registrar_comando("p",    "Modo parada",    cmd_parada);
    consola_registrar_comando("t",    "Modo turnos",    cmd_turnos);
    consola_registrar_comando("i", "Modo inanicion", cmd_inanicion);
    consola_registrar_comando("c", "Modo corregida", cmd_corregida);

    consola_iniciar(6); /* Prioridad 6 de FreeRTOS */
}
