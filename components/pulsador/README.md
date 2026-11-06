# Componente `pulsador` (con antirrebote)

Un pulsador en un pin de entrada digital con soporte para antirrebote por software.

## Características
- Configura el pin como entrada con la resistencia de polarización que corresponda (pull-up / pull-down).
- Permite registrar interrupciones por flanco en el pin.
- Incluye el filtro antirrebote (`filtro.h` / `filtro.c`) con ventana de guarda y confirmación de nivel configurable para rechazar rebotes mecánicos y flancos espurios.

## Uso del pulsador

```c
#include "pulsador.h"
#include "pines.h"

static pulsador_t pulsador;

void app_main(void)
{
    ESP_ERROR_CHECK(pulsador_iniciar(&pulsador, PIN_PULSADOR, PULSADOR_NIVEL_PULSADO));
    if (pulsador_pulsado(&pulsador)) {
        // ...
    }
}
```

## Uso del filtro antirrebote

```c
#include "filtro.h"

static filtro_t filtro = {
    .guarda_us = 50000,        /* 50 ms de tiempo de guarda */
    .confirmar = true,
    .confirmacion_us = 20000,  /* 20 ms para confirmar el nivel */
};

/* En la rutina o tarea que procesa los eventos: */
filtro_reiniciar(&filtro);
char marca = filtro_flanco(&filtro, instante_us, funcion_sigue_pulsado, contexto);
if (marca == MARCA_PULSACION) {
    // Pulsación válida confirmada
}
```
