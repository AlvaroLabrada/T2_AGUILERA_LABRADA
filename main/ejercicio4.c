#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>
#include <stdlib.h>


#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_timer.h"

#include "ejercicio1.h"
#include "ejercicio2.h"
#include "ejercicio3.h"
#include "ejercicio4.h"

static const char *TAG = "consola";

#define LINEA_MAX_LEN 64

/* Firma de las funciones de comando */
typedef void (*comando_fn_t)(char *args);

typedef struct {
    const char   *nombre;
    const char   *ayuda;
    comando_fn_t  ejecutar;
} comando_t;

static void cmd_ayuda(char *args);
static void cmd_estado(char *args);
static void cmd_led(char *args);

/* Tabla de comandos*/
static const comando_t TABLA_COMANDOS[] = {
    { "h",  "Muestra esta lista de comandos",            cmd_ayuda  },
    { "e", "Consulta el estado general del sistema",    cmd_estado },
    { "l",    "Controla el LED (<on|off|auto|<periodo>>)",           cmd_led    },
};
#define NUM_COMANDOS (sizeof(TABLA_COMANDOS) / sizeof(TABLA_COMANDOS[0]))

static void cmd_ayuda(char *args)
{
    (void)args;
    printf("\nComandos disponibles:\n");
    for (size_t i = 0; i < NUM_COMANDOS; i++) {
        printf("  %-8s - %s\n", TABLA_COMANDOS[i].nombre, TABLA_COMANDOS[i].ayuda);
    }
    printf("\n");
}

static void cmd_estado(char *args)
{
    (void)args;
    int64_t t_ms = esp_timer_get_time() / 1000;
    uint32_t pulsaciones = ejercicio2_obtener_pulsaciones();
    bme280_medidas_t m = ejercicio3_obtener_medidas();

    printf("\n[ESTADO DEL SISTEMA]\n");
    printf("  Tiempo de actividad : %lld ms\n", (long long)t_ms);
    printf("  Pulsaciones totales : %" PRIu32 "\n", pulsaciones);

    if (m.valido) {
        printf("  Temperatura   : %.2f C\n", m.temperatura);
        printf("  Presion       : %.2f Pa\n", m.presion);
        printf("  Humedad       : %.2f %%\n", m.humedad);
    } else {
        printf("  Sensor BME280       : Esperando primera lectura...\n");
    }

    printf("  Puerto consola      : UART0 a 115200 baudios\n");
    printf("  Tick de FreeRTOS    : %d Hz\n\n", configTICK_RATE_HZ);
}

/* Subcomandos para el control del LED */
typedef void (*accion_led_fn_t)(void);

static void accion_led_on(void)
{
    ejercicio1_fijar_estado_led(true);
    printf("LED encendido manualmente (parpadeo pausado).\n");
}

static void accion_led_off(void)
{
    ejercicio1_fijar_estado_led(false);
    printf("LED apagado manualmente (parpadeo pausado).\n");
}

static void accion_led_auto(void)
{
    ejercicio1_pausar_parpadeo(false);
    printf("Parpadeo automatico reanudado.\n");
}

typedef struct {
    const char      *subcomando;
    accion_led_fn_t  accion;
} subcomando_led_t;

static const subcomando_led_t TABLA_SUBCOMANDOS_LED[] = {
    { "on",   accion_led_on   },
    { "1",    accion_led_on   },
    { "off",  accion_led_off  },
    { "0",    accion_led_off  },
    { "auto", accion_led_auto },
};
#define NUM_SUBCOMANDOS_LED (sizeof(TABLA_SUBCOMANDOS_LED) / sizeof(TABLA_SUBCOMANDOS_LED[0]))

static void cmd_led(char *args)
{
    if (args == NULL || *args == '\0') {
        printf("Uso:\n");
        printf("  led on          - Enciende fijo\n");
        printf("  led off         - Apaga fijo\n");
        printf("  led auto        - Reanuda parpadeo\n");
        printf("  led <ms>        - Cambia periodo en ms (ej: led 200)\n");
        return;
    }

    /* 1. Comprobar si se ha introducido un número para el período */
    char *endptr;
    unsigned long periodo = strtoul(args, &endptr, 10);
    if (endptr != args && periodo > 0) {
        ejercicio1_cambiar_periodo((uint32_t)periodo);
        printf("Periodo del LED cambiado a %lu ms.\n", periodo);
        return;
    }

    /* 2. Si no es un número, comprobar la subtabla de subcomandos */
    for (size_t i = 0; i < NUM_SUBCOMANDOS_LED; i++) {
        if (strcmp(args, TABLA_SUBCOMANDOS_LED[i].subcomando) == 0) {
            TABLA_SUBCOMANDOS_LED[i].accion();
            return;
        }
    }

    printf("Parametro no valido '%s'. Use: on, off, auto o un numero en ms.\n", args);
}

static void despachar_linea(char *linea)
{
    while (*linea == ' ' || *linea == '\t') linea++;

    if (*linea == '\0') {
        return;
    }

    char *cmd = strtok(linea, " \t");
    char *args = strtok(NULL, "");

    for (size_t i = 0; i < NUM_COMANDOS; i++) {
        if (strcmp(cmd, TABLA_COMANDOS[i].nombre) == 0) {
            TABLA_COMANDOS[i].ejecutar(args);
            return;
        }
    }

    printf("Comando no reconocido: '%s'. Escriba 'h' o 'ayuda' para ver la lista.\n", cmd);
}

/* Tarea de recepción. Basado en la técnica de lectura de consola de tarea_teclado (salida_y_temporizacion.c) */
static void tarea_consola(void *arg)
{
    (void)arg;
    char buffer[LINEA_MAX_LEN];
    size_t indice = 0;
    bool linea_descartada = false;

    printf("\n=== Consola serie lista (escriba 'h' y pulse Intro) ===\n> ");
    fflush(stdout);

    for (;;) {
        char tecla;
        /* Lectura directa idéntica a la tarea_teclado del profesor */
        if (read(STDIN_FILENO, &tecla, 1) == 1) {

            /* Fin de línea al pulsar Intro: \r (monitor de ESP-IDF) o \n */
            if (tecla == '\r' || tecla == '\n') {
                if (linea_descartada) {
                    printf("\nError: linea demasiado larga descartada (max %d caracteres).\n> ", LINEA_MAX_LEN - 1);
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

            /* Descarte automático si la línea es demasiado larga (requisito P2) */
            if (indice >= (LINEA_MAX_LEN - 1)) {
                linea_descartada = true;
                continue;
            }

            /* Acumular carácter introducido */
            if (!linea_descartada) {
                buffer[indice++] = tecla;
                putchar(tecla);
                fflush(stdout);
            }
        }

        /* Espera no bloqueante*/
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

void run_ejercicio4(void)
{
    /* Prioridad 6 (superior a la del LED) para atender el teclado sin retrasos */
    const BaseType_t res = xTaskCreate(tarea_consola, "consola", 4096, NULL, 6, NULL);
    if (res != pdPASS) {
        ESP_LOGE(TAG, "No hay memoria para crear la tarea de consola");
        return;
    }

    ESP_LOGI(TAG, "Consola lista sobre UART0.");
}