#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "pines.h"
#include "bme280_port.h"
#include "consola.h"
#include "ejercicio1.h"
#include "ejercicio2.h"
#include <inttypes.h>

#define PRIORIDAD_SENSOR 4  
#define PERIODO_SENSOR_MS      100  /* Periodo de muestreo*/
#define MUESTRAS_OBJETIVO      50   /* Ventana de 50 activaciones para el resumen */

static const char *TAG = "sensor";

static const char *NOMBRES_MODOS[4] = { "PARADA", "TURNOS", "INANICION", "CORREGIDA" };

/* [REUTILIZADO]: Cerrojo crítico para proteger las variables compartidas */
static portMUX_TYPE s_cerrojo_medidas = portMUX_INITIALIZER_UNLOCKED;

typedef struct {
    uint32_t n;              /* Cuántas activaciones se han medido */
    int64_t  suma_us;        /* Suma de intervalos (para la media) */
    int64_t  max_us;         /* Intervalo más largo observado */
    int64_t  min_us;         /* Intervalo más corto observado */
    int64_t  anterior_us;    /* Marca de tiempo de la activación previa */
    uint32_t rebases;        /* Activaciones que se retrasaron (pdFALSE en delay) */
} estadistica_sensor_t;
static estadistica_sensor_t s_stats = { 0 };

/* Array donde guardamos un resumen por cada modo (Paso 6) */
static estadistica_sensor_t s_resumen_modo[4] = { 0 };

/* [REUTILIZADO]: Formateador de microsegundos a milisegundos con coma decimal */
static void escribir_ms(char *destino, size_t tam, int64_t us)
{
    if (us < 0) us = 0; /*Impide valores negativos por si hay un fallo*/
    snprintf(destino, tam, "%" PRId64 ",%" PRId64, us / 1000, (us % 1000) / 100); //Escribir en ms con 1 decimal
    /*%" PRId64 ": Significa: «imprime un entero de 64 bits con signo (int64_t) de forma portable»*/
}

/* [REUTILIZADO de anotar_conmutacion_led]: Calcula intervalo, min, max y media */
static void anotar_activacion(int64_t ahora, bool rebase) {
    portENTER_CRITICAL(&s_cerrojo_medidas);
    if (rebase) {
        s_stats.rebases++;
    }

    if (s_stats.anterior_us != 0) {
        const int64_t d = ahora - s_stats.anterior_us;
        s_stats.n++;
        s_stats.suma_us += d;
        if (d > s_stats.max_us) s_stats.max_us = d;
        if (s_stats.min_us == 0 || d < s_stats.min_us) s_stats.min_us = d;
    }

    s_stats.anterior_us = ahora;
    portEXIT_CRITICAL(&s_cerrojo_medidas);
}
/* [REUTILIZADO de mostrar_resumen]: Formatea con escribir_ms e imprime */
static void mostrar_resumen_sensor(modo_carga_t modo) {
    estadistica_sensor_t copia;

    portENTER_CRITICAL(&s_cerrojo_medidas);
    copia = s_stats; //Copia local para poder imprimir por la terminal sin bloquear el procesador
    s_resumen_modo[modo] = s_stats;
    portEXIT_CRITICAL(&s_cerrojo_medidas);

    if (copia.n == 0) return;
    
    char a[16], b[16], c[16], j[16];
    int64_t jitter_us = copia.max_us - copia.min_us;
    
    escribir_ms(a, sizeof(a), copia.suma_us / copia.n);
    escribir_ms(b, sizeof(b), copia.min_us);
    escribir_ms(c, sizeof(c), copia.max_us);
    escribir_ms(j, sizeof(j), jitter_us);

    printf("\n--- [RESUMEN SENSOR BME280 (%s): %" PRIu32 " activaciones] ---\n", NOMBRES_MODOS[modo], copia.n);
    printf("    Periodo nominal: %d ms\n", PERIODO_SENSOR_MS);
    printf("    Intervalo medio: %s ms, minimo %s ms, maximo %s ms\n", a, b, c);
    printf("    Jitter (Max - Min): %s ms (%" PRId64 " us)\n", j, jitter_us);
    printf("    Rebases de plazo  : %" PRIu32 "\n\n", copia.rebases);
}

static void cmd_ultimo_resumen(char *args) {
    printf("\n--- [ULTIMO RESUMEN DE CADA MODO] ---\n");
    for (int i = 0; i < 4; i++) {
        if (s_resumen_modo[i].n > 0) {
            char a[16], b[16], c[16], j[16];
            int64_t jitter = s_resumen_modo[i].max_us - s_resumen_modo[i].min_us;
            escribir_ms(a, sizeof(a), s_resumen_modo[i].suma_us / s_resumen_modo[i].n);
            escribir_ms(b, sizeof(b), s_resumen_modo[i].min_us);
            escribir_ms(c, sizeof(c), s_resumen_modo[i].max_us);
            escribir_ms(j, sizeof(j), jitter);
            printf("  [%-10s] Medio %s ms, Min %s ms, Max %s ms, Jitter %s ms (%" PRId64 " us), Rebases %" PRIu32 "\n",
                   NOMBRES_MODOS[i], a, b, c, j, jitter,s_resumen_modo[i].rebases);
        } else {
            printf("  [%-10s] Sin resumen todavia\n", NOMBRES_MODOS[i]);
        }
    }
    printf("\n");
}

static void tarea_sensor(void *arg) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xPeriodo = pdMS_TO_TICKS(PERIODO_SENSOR_MS);
    modo_carga_t modo_anterior = carga_obtener_modo();

    while (1) {
        // Espera periódica absoluta (devuelve pdFALSE si hubo rebase)
        bool rebase = (xTaskDelayUntil(&xLastWakeTime, xPeriodo) == pdFALSE);
        // 1. Marca de tiempo antes de leer el sensor
        int64_t ahora = esp_timer_get_time();

        // 2. Si el modo de carga ha cambiado, descartar lo acumulado y reiniciar
        modo_carga_t modo_actual = carga_obtener_modo();
        if (modo_actual != modo_anterior) {
            modo_anterior = modo_actual;

            portENTER_CRITICAL(&s_cerrojo_medidas);
            s_stats = (estadistica_sensor_t){ 0 }; // Descarta muestras
            portEXIT_CRITICAL(&s_cerrojo_medidas);
        }
        // 3. Anotar la activación (función reutilizada con cerrojo crítico)
        anotar_activacion(ahora, rebase);

        // 4. Lectura física del sensor BME280
        bme280_datos_t datos;
        bme280_port_leer(&datos);

        // 5. Al llegar a 50 activaciones, muestra el resumen y reinicia las estadísticas
        if (s_stats.n >= MUESTRAS_OBJETIVO) {
            mostrar_resumen_sensor(modo_actual);
            portENTER_CRITICAL(&s_cerrojo_medidas);
            s_stats = (estadistica_sensor_t){ 0 };
            portEXIT_CRITICAL(&s_cerrojo_medidas);
        }
    }
}


void run_ejercicio2(void) {
    ESP_LOGI(TAG, "Iniciando sensor BME280 y tarea de medida periodica...");
    bme280_port_iniciar(PIN_I2C_SDA, PIN_I2C_SCL, BME280_I2C_ADDR);

    consola_registrar_comando("r",    "Ultimo resumen de cada modo",    cmd_ultimo_resumen);
    
    xTaskCreatePinnedToCore(tarea_sensor, "Sensor", 3072, NULL, PRIORIDAD_SENSOR, NULL, 0);
}