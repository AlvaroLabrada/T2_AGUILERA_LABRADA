#pragma once
#include <stdbool.h>

typedef struct {
    float temperatura;
    float presion;
    float humedad;
    bool  valido;
} bme280_medidas_t;

void run_ejercicio3(void);
bme280_medidas_t ejercicio3_obtener_medidas(void);