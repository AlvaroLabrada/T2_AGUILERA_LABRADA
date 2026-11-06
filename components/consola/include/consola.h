/* consola.h - Componente de consola serie interactiva por UART / STDIN.
 *
 * Proporciona recepción de comandos, eco por terminal, control de longitud de
 * línea (máximo 64 caracteres) y despacho mediante tabla de comandos.
 */
#pragma once

#include <stddef.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CONSOLA_LINEA_MAX_LEN 64
#define CONSOLA_MAX_COMANDOS  16

/* Firma de las funciones de comando: recibe la cadena con los argumentos */
typedef void (*comando_fn_t)(char *args);

typedef struct {
    const char   *nombre;
    const char   *ayuda;
    comando_fn_t  ejecutar;
} comando_t;

/**
 * @brief Registra un comando en la consola.
 *
 * @param nombre Nombre/identificador del comando (ej: "h", "e", "l").
 * @param ayuda Descripción mostrada en la ayuda.
 * @param ejecutar Función callback encargada de ejecutarlo.
 * @return ESP_OK si se registró con éxito, ESP_ERR_NO_MEM si la tabla está llena.
 */
esp_err_t consola_registrar_comando(const char *nombre, const char *ayuda, comando_fn_t ejecutar);

/**
 * @brief Inicia la tarea de recepción de la consola en segundo plano.
 *
 * Crea la tarea de FreeRTOS que lee de STDIN con eco y despacha los comandos.
 * El comando de ayuda 'h' viene registrado por defecto.
 *
 * @param prioridad Prioridad FreeRTOS para la tarea de consola (ej. 6).
 * @return ESP_OK si la tarea se creó correctamente, o error de FreeRTOS.
 */
esp_err_t consola_iniciar(int prioridad);

#ifdef __cplusplus
}
#endif

