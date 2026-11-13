#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#define MUESTRAS_OBJETIVO 50 /* Ventana de 50 activaciones para el resumen */

/* Estructura compartida para P2 y P4 */
typedef struct {
    uint32_t n;              /* Cuántas activaciones se han medido */
    int64_t  suma_us;        /* Suma de intervalos (para la media) */
    int64_t  max_us;         /* Intervalo más largo observado */
    int64_t  min_us;         /* Intervalo más corto observado */
    int64_t  anterior_us;    /* Marca de tiempo de la activación previa */
    uint32_t rebases;        /* Activaciones que llegaron tarde */
} estadistica_sensor_t;

/* Herramientas compartidas */
extern const char *NOMBRES_MODOS[4];
void escribir_ms(char *destino, size_t tam, int64_t us);
void estadistica_anotar(estadistica_sensor_t *st, int64_t ahora, bool rebase);

void run_ejercicio2(void);