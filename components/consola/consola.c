/* consola.c - Implementación del componente consola interactiva por UART0 / STDIN. */

#include "consola.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "consola";

static comando_t s_comandos[CONSOLA_MAX_COMANDOS];
static size_t    s_num_comandos = 0;

static void cmd_ayuda_builtin(char *args)
{
    (void)args;
    printf("\nComandos disponibles:\n");
    for (size_t i = 0; i < s_num_comandos; i++) {
        printf("  %-8s - %s\n", s_comandos[i].nombre, s_comandos[i].ayuda);
    }
    printf("\n");
}

esp_err_t consola_registrar_comando(const char *nombre, const char *ayuda, comando_fn_t ejecutar)
{
    if (nombre == NULL || ejecutar == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    /* Si ya existe un comando con ese nombre, actualizarlo */
    for (size_t i = 0; i < s_num_comandos; i++) {
        if (strcmp(s_comandos[i].nombre, nombre) == 0) {
            s_comandos[i].ayuda = ayuda;
            s_comandos[i].ejecutar = ejecutar;
            return ESP_OK;
        }
    }
    if (s_num_comandos >= CONSOLA_MAX_COMANDOS) {
        ESP_LOGE(TAG, "Tabla de comandos llena (max %d)", CONSOLA_MAX_COMANDOS);
        return ESP_ERR_NO_MEM;
    }

    s_comandos[s_num_comandos++] = (comando_t){
        .nombre = nombre,
        .ayuda = ayuda,
        .ejecutar = ejecutar,
    };
    return ESP_OK;
}

static void despachar_linea(char *linea)
{
    while (*linea == ' ' || *linea == '\t') linea++;

    if (*linea == '\0') {
        return;
    }

    char *cmd = strtok(linea, " \t");
    char *args = strtok(NULL, "");

    for (size_t i = 0; i < s_num_comandos; i++) {
        if (strcmp(cmd, s_comandos[i].nombre) == 0) {
            s_comandos[i].ejecutar(args);
            return;
        }
    }

    printf("Comando no reconocido: '%s'. Escriba 'h' para ver la lista.\n", cmd);
}

static void tarea_consola(void *arg)
{
    (void)arg;
    char buffer[CONSOLA_LINEA_MAX_LEN];
    size_t indice = 0;
    bool linea_descartada = false;

    printf("\n=== Consola serie lista (escriba 'h' y pulse Intro) ===\n> ");
    fflush(stdout);

    for (;;) {
        char tecla;
        if (read(STDIN_FILENO, &tecla, 1) == 1) {
            /* Fin de línea al pulsar Intro: \r (monitor ESP-IDF) o \n */
            if (tecla == '\r' || tecla == '\n') {
                if (linea_descartada) {
                    printf("\nError: linea demasiado larga descartada (max %d caracteres).\n> ",
                           CONSOLA_LINEA_MAX_LEN - 1);
                    linea_descartada = false;
                    indice = 0;
                } else if (indice > 0) {
                    buffer[indice] = '\0';
                    printf("\n");
                    despachar_linea(buffer);
                    indice = 0;
                    printf("> ");
                } else {
                    printf("\n> ");
                }
                fflush(stdout);
                continue;
            }

            /* Descarte automático si la línea supera el límite */
            if (indice >= (CONSOLA_LINEA_MAX_LEN - 1)) {
                linea_descartada = true;
                continue;
            }

            /* Acumular carácter con eco */
            if (!linea_descartada) {
                buffer[indice++] = tecla;
                putchar(tecla);
                fflush(stdout);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

esp_err_t consola_iniciar(int prioridad)
{
    /* Registrar el comando de ayuda interno 'h' si no está registrado */
    consola_registrar_comando("h", "Muestra esta lista de comandos", cmd_ayuda_builtin);

    const BaseType_t res = xTaskCreate(tarea_consola, "consola", 4096, NULL, prioridad, NULL);
    if (res != pdPASS) {
        ESP_LOGE(TAG, "No hay memoria para crear la tarea de consola");
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "Consola serie iniciada con prioridad %d.", prioridad);
    return ESP_OK;
}

