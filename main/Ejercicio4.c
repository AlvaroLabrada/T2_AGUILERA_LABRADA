#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"

#include <inttypes.h>

#include "ejercicio1.h" // Para consultar carga_obtener_modo() 
#include "ejercicio2.h"
#include "ejercicio4.h"

#include "freertos/timers.h"
#include "freertos/queue.h"

#define PERIODO_INICIAL_MS 10  // 10 ms

typedef enum {
    TIMER_ESP = 0,  // Alarma de esp_timer 
    TIMER_SW  = 1   // Temporizador software de FreeRTOS
} origen_timer_t;

typedef struct { //Estructura que se envía a la cola cuando un temporizador se dispara
    origen_timer_t origen;
    int64_t        instante_us;
} evento_timer_t;

static QueueHandle_t     s_cola_timers = NULL;
static volatile uint32_t s_perdidas[2] = { 0, 0 }; // Array de 2 contadores de eventos perdidos debido a la cola llena (uno por cada temporizador)
static volatile bool     s_reiniciar_perdida[2] = { false, false }; // Array de 2 banderas que indican si se debe reiniciar el contador de eventos perdidos (uno por cada temporizador)

static const char *TAG = "temporizadores";
static esp_timer_handle_t s_alarma_esp = NULL; //Identificador de la alarma esp_timer que comieza vacía 
static TimerHandle_t s_timer_sw = NULL; // Identificador del temporizador software de FreeRTOS

static int s_periodo_actual_ms = PERIODO_INICIAL_MS;
static estadistica_sensor_t s_stats[2] = { 0 };
static const char *NOMBRES_TIMERS[2] = { "esp_timer", "FreeRTOS_SW" };


static void enviar_marca(origen_timer_t origen)
{
    evento_timer_t ev = { //Estructura que se envía a la cola cuando un temporizador se dispara
        .origen      = origen,
        .instante_us = esp_timer_get_time(),
    };

    // 0 ticks de espera: nunca se bloquea 
    // Introduce una copia de ev en la cola s_cola_timers
    // Si la cola está llena, incrementa el contador de eventos perdidos y marca que se debe reiniciar el contador
    if (xQueueSend(s_cola_timers, &ev, 0) != pdTRUE) {
        s_perdidas[origen]++;
        s_reiniciar_perdida[origen] = true;
    }
}

static void respuesta_alarma(void *arg)
{
    (void)arg;
    enviar_marca(TIMER_ESP); //Indica que la alarma esp_timer se ha disparado y envía un evento a la cola
}

static void respuesta_timer(TimerHandle_t temporizador)
{
    (void)temporizador;
    enviar_marca(TIMER_SW); //Indica que el temporizador software de FreeRTOS se ha disparado y envía un evento a la cola
}

static void mostrar_resumen_timer(origen_timer_t origen, modo_carga_t modo)
{
    estadistica_sensor_t *st = &s_stats[origen];
    if (st->n == 0) return;

    char a[16], b[16], c[16], j[16];
    int64_t jitter_us = st->max_us - st->min_us;

    escribir_ms(a, sizeof(a), st->suma_us / st->n);
    escribir_ms(b, sizeof(b), st->min_us);
    escribir_ms(c, sizeof(c), st->max_us);
    escribir_ms(j, sizeof(j), jitter_us);

    printf("\n--- [TIMER %-11s | Modo: %-9s | N=%" PRIu32 "] ---\n",
           NOMBRES_TIMERS[origen], NOMBRES_MODOS[modo], st->n);
    printf("    Medio: %s ms | Min: %s ms | Max: %s ms\n", a, b, c);
    printf("    Jitter: %s ms (%" PRId64 " us) | Rebases: %" PRIu32 " | Perdidas cola: %" PRIu32 "\n",
           j, jitter_us, st->rebases, s_perdidas[origen]);
}

static void tarea_medida_timers(void *arg)
{
    evento_timer_t ev;
    modo_carga_t modo_anterior = carga_obtener_modo();

    while (1) {
        if (xQueueReceive(s_cola_timers, &ev, portMAX_DELAY) == pdTRUE) {

            /* Si cambió el modo de carga, descartar medidas acumuladas */
            modo_carga_t modo_actual = carga_obtener_modo();
            if (modo_actual != modo_anterior) {
                modo_anterior = modo_actual;
                s_stats[TIMER_ESP] = (estadistica_sensor_t){ 0 };
                s_stats[TIMER_SW]  = (estadistica_sensor_t){ 0 };
            }

            /* Si hubo pérdida en la cola, empezar de nuevo para ese timer */
            if (s_reiniciar_perdida[ev.origen]) {
                s_reiniciar_perdida[ev.origen] = false;
                s_stats[ev.origen] = (estadistica_sensor_t){ 0 };
            }

            /* Comprobamos si el intervalo superó el periodo (+1 ms de margen) */
            bool rebase = false;
            if (s_stats[ev.origen].anterior_us != 0) {
                int64_t delta = ev.instante_us - s_stats[ev.origen].anterior_us;
                if (delta > (s_periodo_actual_ms * 1000LL + 1000LL)) {
                    rebase = true;
                }
            }

            /* Reutilización de la función de ejercicio2.h */
            estadistica_anotar(&s_stats[ev.origen], ev.instante_us, rebase);

            /* Al llegar a 50 muestras, mostrar resumen y reiniciar */
            if (s_stats[ev.origen].n >= MUESTRAS_OBJETIVO) {
                mostrar_resumen_timer(ev.origen, modo_actual);
                s_stats[ev.origen] = (estadistica_sensor_t){ 0 };
            }
        }
    }
}

void run_ejercicio4(void)
{
    ESP_LOGI(TAG, "Iniciando Ejercicio 4 (Periodo inicial: %d ms)...", PERIODO_INICIAL_MS);
    
    s_cola_timers = xQueueCreate(32, sizeof(evento_timer_t)); // Cola de 32 bits para recibir eventos de los temporizadores 
    
    xTaskCreatePinnedToCore(tarea_medida_timers, "MedidaTimers", 3072, NULL, 3, NULL, 1); // Tarea de medida fijada al Núcleo 1

    // Estructura de configuración de la alarma esp_timer
    const esp_timer_create_args_t args_alarma = {
        .callback        = respuesta_alarma, // Función que se ejecuta cuando la alarma se dispara
        .dispatch_method = ESP_TIMER_TASK, // Ejecuta en la tarea interna esp_timer (prioridad 22)
        .name            = "alarma",
    };

    ESP_ERROR_CHECK(esp_timer_create(&args_alarma, &s_alarma_esp)); // Crea la alarma esp_timer con la configuración especificada y guarda la dirección en s_alarma_esp
    ESP_ERROR_CHECK(esp_timer_start_periodic(s_alarma_esp, PERIODO_INICIAL_MS * 1000ULL)); //Multiplica por 1000 para convertir milisegundos a microsegundos
    
    // Temporizador software de FreeRTOS
    s_timer_sw = xTimerCreate(
        "temporizador_sw",                // Nombre para depuración
        pdMS_TO_TICKS(PERIODO_INICIAL_MS),// Periodo expresado en ticks (10 ms)
        pdTRUE,                           // pdTRUE = periódico (se repite solo)
        NULL,                             // ID opcional (no lo usamos)
        respuesta_timer                   // Función de respuesta al vencer
    );

    xTimerStart(s_timer_sw, portMAX_DELAY); // Arranca el temporizador software
}