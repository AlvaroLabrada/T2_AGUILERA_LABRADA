#pragma once
#include <stdbool.h>
#include <stdint.h>
void run_ejercicio1(void);

/* Funciones que necesita la consola para controlar el LED */
void ejercicio1_fijar_estado_led(bool encender);
void ejercicio1_pausar_parpadeo(bool pausar);
void ejercicio1_cambiar_periodo(uint32_t nuevo_periodo_ms);