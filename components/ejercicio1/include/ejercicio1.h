#pragma once
#include <stdbool.h>
#include <stdint.h>

typedef enum {
    MODO_PARADA = 0,
    MODO_TURNOS,
    MODO_INANICION,
    MODO_CORREGIDA
} modo_carga_t;

/* Devuelve el modo actual de la carga */
modo_carga_t carga_obtener_modo(void);

void run_ejercicio1(void);



/* Funciones que necesita la consola para controlar el LED */
//void ejercicio1_fijar_estado_led(bool encender);
//void ejercicio1_pausar_parpadeo(bool pausar);
//void ejercicio1_cambiar_periodo(uint32_t nuevo_periodo_ms);