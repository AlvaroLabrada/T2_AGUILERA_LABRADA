# Componente `consola`

Consola serie interactiva sobre UART0 / STDIN.

## Características
- Entrada no bloqueante mediante lectura directa de `STDIN_FILENO`.
- Eco inmediato de caracteres en el terminal.
- Límite de línea de 64 caracteres: las líneas que superan este tamaño se descartan avisando al usuario.
- Despacho modular mediante tabla dinámica de comandos.
- Comando `h` integrado para mostrar la lista de comandos disponibles.

## Uso básico

```c
#include "consola.h"

static void cmd_hola(char *args)
{
    printf("¡Hola %s!\n", (args && *args) ? args : "mundo");
}

void app_main(void)
{
    consola_registrar_comando("hola", "Saluda al usuario", cmd_hola);
    consola_iniciar(6); /* Prioridad 6 de FreeRTOS */
}
```

