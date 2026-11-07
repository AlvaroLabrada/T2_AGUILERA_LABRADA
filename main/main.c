#include <stdio.h>
#include <string.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "pines.h"
#include "consola.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "main";

/* 1. Definición de los 4 modos de carga */
typedef enum {
    MODO_PARADA = 0,    /* Las dos tareas sin calcular */
    MODO_TURNOS,        /* Misma prioridad (2), 20 ms cálculo y 27 ms espera */
    MODO_INANICION,     /* Tarea 1 a prioridad 3 sin espera (acapara el núcleo) */
    MODO_CORREGIDA      /* Tarea 1 a prioridad 3 pero recupera la espera de 27 ms */
} modo_carga_t;

static volatile modo_carga_t s_modo = MODO_PARADA;

static volatile uint32_t contador_carga1 = 0;
static volatile uint32_t contador_carga2 = 0;

/* Handles para poder cambiar prioridades dinámicamente */
static TaskHandle_t handle_tarea1 = NULL;
static TaskHandle_t handle_tarea2 = NULL;

/* 2. Tareas de carga fijadas al Núcleo 0 */
static void tarea_carga_1(void *arg) {
    while (1) {
        if (s_modo == MODO_PARADA) {
            vTaskDelay(pdMS_TO_TICKS(50));
            continue;
        }

        // Ráfaga de cálculo de 20 ms de CPU activa
        int64_t inicio = esp_timer_get_time();
        while ((esp_timer_get_time() - inicio) < 20000) {
        }
        contador_carga1++;

        // En inanición NO se bloquea (0 ms de espera). En turnos y corregida espera 27 ms
        if (s_modo != MODO_INANICION) {
            vTaskDelay(pdMS_TO_TICKS(27));
        }
    }
}

static void tarea_carga_2(void *arg) {
    while (1) {
        if (s_modo == MODO_PARADA) {
            vTaskDelay(pdMS_TO_TICKS(50));
            continue;
        }

        // Ráfaga de cálculo de 20 ms de CPU activa
        int64_t inicio = esp_timer_get_time();
        while ((esp_timer_get_time() - inicio) < 20000) {
        }
        contador_carga2++;

        vTaskDelay(pdMS_TO_TICKS(27));
    }
}

/* 3. Tarea de informe (fijada al Núcleo 1) */
static void tarea_informe(void *arg) {
    uint32_t prev1 = 0;
    uint32_t prev2 = 0;

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));

        uint32_t c1 = contador_carga1;
        uint32_t c2 = contador_carga2;

        uint32_t delta1 = c1 - prev1;
        uint32_t delta2 = c2 - prev2;
        prev1 = c1;
        prev2 = c2;

        const char *nombre_modo = (s_modo == MODO_PARADA)    ? "PARADA" :
                                  (s_modo == MODO_TURNOS)    ? "TURNOS" :
                                  (s_modo == MODO_INANICION) ? "INANICION" : "CORREGIDA";

        ESP_LOGI(TAG, "[%s] Carga 1: +%3lu it/s (tot: %lu) | Carga 2: +%3lu it/s (tot: %lu)",
                 nombre_modo,
                 (unsigned long)delta1, (unsigned long)c1,
                 (unsigned long)delta2, (unsigned long)c2);
    }
}

/* 4. Función del comando de consola para cambiar de modo */
static void cmd_carga(char *args) {
    if (args == NULL || *args == '\0') {
        printf("Uso: carga <parada|turnos|inanicion|corregida>\n");
        return;
    }

    if (strcmp(args, "parada") == 0 || strcmp(args, "0") == 0) {
        s_modo = MODO_PARADA;
        vTaskPrioritySet(handle_tarea1, 2);
        vTaskPrioritySet(handle_tarea2, 2);
        printf("Carga -> PARADA (tareas inactivas sin calcular)\n");
    } else if (strcmp(args, "turnos") == 0 || strcmp(args, "1") == 0) {
        s_modo = MODO_TURNOS;
        vTaskPrioritySet(handle_tarea1, 2);
        vTaskPrioritySet(handle_tarea2, 2);
        printf("Carga -> TURNOS (ambas a Prio 2, reparto equitativo)\n");
    } else if (strcmp(args, "inanicion") == 0 || strcmp(args, "2") == 0) {
        s_modo = MODO_INANICION;
        vTaskPrioritySet(handle_tarea1, 3); // Tarea 1 sube a Prio 3
        vTaskPrioritySet(handle_tarea2, 2); // Tarea 2 se queda a Prio 2
        printf("Carga -> INANICION (Tarea 1 a Prio 3 sin espera; aviso Watchdog en ~5s)\n");
    } else if (strcmp(args, "corregida") == 0 || strcmp(args, "3") == 0) {
        s_modo = MODO_CORREGIDA;
        vTaskPrioritySet(handle_tarea1, 3); // Mantiene Prio 3
        vTaskPrioritySet(handle_tarea2, 2); // Tarea 2 a Prio 2
        printf("Carga -> CORREGIDA (Tarea 1 a Prio 3 con espera de 27ms recuperada)\n");
    } else {
        printf("Modo no reconocido: '%s'. Opciones: parada, turnos, inanicion, corregida\n", args);
    }
}

void app_main(void) {
    ESP_LOGI(TAG, "Arrancando Taller 2 - P1 con control de modos por consola");

    /* Ambas tareas de carga fijadas al Núcleo 0 (prioridad inicial 2) */
    xTaskCreatePinnedToCore(tarea_carga_1, "Tarea1", 3072, NULL, 2, &handle_tarea1, 0);
    xTaskCreatePinnedToCore(tarea_carga_2, "Tarea2", 3072, NULL, 2, &handle_tarea2, 0);

    /* Tarea de informe fijada al Núcleo 1 (prioridad 3) */
    xTaskCreatePinnedToCore(tarea_informe, "Informe", 3072, NULL, 3, NULL, 1);

    /* Registrar el comando 'carga' e iniciar la consola con prioridad 6 */
    consola_registrar_comando("carga", "Cambia modo de carga: parada, turnos, inanicion, corregida", cmd_carga);
    consola_iniciar(6);
}
