# Taller 2 — FreeRTOS: tareas, prioridades y temporización

SEyTR 2026-27 · Máster en Robótica y Automática · UCLM

## Integrantes

- Heysser Aguilera
- Álvaro Labrada Isidro

Reparto: P1 y P4 las hicimos entre los dos, P2 Álvaro y P3 Heysser.

## Pines y montaje

| Elemento | Pin | Notas |
|---|---|---|
| LED | GPIO5 | Con resistencia a masa, se enciende con 1 |
| Pulsador | GPIO4 | A masa con pull-up externa, pulsado = 0 |
| BME280 SDA | GPIO8 | I2C, dirección 0x76 |
| BME280 SCL | GPIO9 | I2C a 100 kHz |
| Potenciómetro | GPIO1 | Reservado para la extensión de P3, no se usa |
| Consola | UART0 | USB-UART de la placa, 115200 baudios |

Placa ESP32-S3-DevKitC-1. El LED va de GPIO5 a masa a través de su resistencia, el pulsador entre GPIO4 y masa con una resistencia de pull-up a 3V3, y el BME280 se alimenta a 3V3 y se conecta por I2C a GPIO8/GPIO9. Todos los pines están definidos en `main/pines.h`.

## Compilar y grabar

```bash
idf.py set-target esp32s3
idf.py build
idf.py -p <puerto> flash monitor
```

`sdkconfig.defaults` activa las estadísticas de FreeRTOS (`vTaskList`, `vTaskGetRunTimeStats`) y fija la tarea de servicio de temporizadores al núcleo 0 (`CONFIG_FREERTOS_TIMER_TASK_AFFINITY_CPU0=y`), que es lo que pide P4.

## Estructura

- `main/Ejercicio1.c` … `Ejercicio4.c`: una práctica por fichero, `main.c` las arranca en orden.
- `components/`: `led`, `pulsador` (con el antirrebote), `bme280` (API de Bosch + puerto I2C) y `consola`.

## Tareas

| Tarea | Prioridad | Núcleo | Qué hace |
|---|:---:|:---:|---|
| consola | 6 | cualquiera | Lee y ejecuta comandos |
| Sensor | 4 | 0 | Lee el BME280 cada 100 ms |
| Tarea1 | 3 (2 en turnos) | 0 | Carga: 20 ms de cálculo + 27 ms de espera |
| Tarea2 | 2 | 0 | Igual que Tarea1 |
| Pulsador | 3 | 0 | Antirrebote y cambio de modo del LED |
| MedidaTimers | 3 | 1 | Mide periodo y jitter de los dos temporizadores |
| Informe | 1 | 1 | Imprime los contadores cada segundo |
| esp_timer (sistema) | 22 | 0 | Respuesta de la alarma esp_timer |
| Tmr Svc (sistema) | 1 | 0 | Respuesta del temporizador de FreeRTOS |

Todas las tareas propias tienen 3072 bytes de pila, menos la consola (4096).

## Comandos de consola

| Comando | Qué hace |
|---|---|
| `h` | Lista los comandos |
| `p` / `t` / `i` / `c` | Modo de carga: parada, turnos, inanición, corregida |
| `r` | Último resumen del sensor en cada modo |
| `periodo <ms>` | Cambia el periodo de los dos temporizadores |
| `resumen` | Resumen actual de los temporizadores |
| `led <0-100>` | Brillo del LED en % |

## Justificaciones

**Periodo del sensor (100 ms).** La medida del BME280 tarda unos 9 ms, así que sobra tiempo. Con 100 ms el resumen de 50 muestras sale cada 5 s y el sensor coincide a menudo con las ráfagas de carga, así se ve bien el efecto de la prioridad.

**Prioridades.** La consola es la más alta para que responda aunque haya carga; casi siempre está bloqueada. El sensor (4) va por encima de las cargas para que no le retrasen. Las cargas (3 y 2) son el experimento de P1. Pulsador y MedidaTimers (3) solo despiertan con eventos y trabajan poco. El informe (1) puede esperar.

**LEDC: 5 kHz y 13 bits.** A partir de unos 100 Hz el ojo no ve parpadeo. Con el reloj de 80 MHz a 5 kHz hay 16 000 pasos por periodo, caben 13 bits (8192) pero no 14 (16 384).

## P1 — Carga, turnos e inanición

Dos tareas de carga en el núcleo 0 (20 ms de cálculo y 27 ms de espera) y una de informe en el núcleo 1 que imprime sus contadores cada segundo. El modo se cambia desde la consola.

- Turnos ([1](resultados/P1.3_turnos_inicio.png), [2](resultados/P1.3_turnos_regimen.png)): los dos contadores suben igual, unos 21 por segundo (1000 / 47 ms).
- Inanición ([watchdog](resultados/P1.4_inanicion_watchdog.png)): Tarea1 con prioridad 3 y sin espera no suelta el núcleo 0, IDLE0 no se ejecuta y el watchdog avisa cada 5 s. Esta traza es de antes de pasar el código de `main.c` a `Ejercicio1.c`, por eso sale `main.c:21`.
- Corregida ([captura](resultados/P1.5_corregida.png)): con la espera de vuelta, los dos contadores avanzan igual y el watchdog deja de saltar.

Lo que no funcionó / terminado después: todo funcionó en el taller. Después solo se movió el código de `main.c` a `Ejercicio1.c`.

## P2 — Sensor periódico

La tarea del sensor usa `xTaskDelayUntil()` para que los retrasos no se acumulen. Toma la marca de tiempo antes de leer y cada 50 lecturas imprime media, mínimo, máximo y jitter. Al cambiar de modo de carga se empieza de cero.

| Prioridad del sensor | Carga | Medio | Mín | Máx | Jitter |
|---|---|---|---|---|---|
| Inferior ([captura](resultados/P2_sensor_prio_inferior_parada.png)) | No | 100,0 | 99,9 | 100,0 | 28 µs |
| Inferior ([captura](resultados/P2_sensor_prio_inferior_turnos.png)) | Sí | 99,7 | 93,9 | 127,0 | 33 ms |
| Superior ([captura](resultados/P2_sensor_prio_superior_parada.png)) | No | 100,0 | 100,0 | 100,0 | 0 µs |
| Superior ([captura](resultados/P2_sensor_prio_superior_turnos.png)) | Sí | 100,0 | 99,9 | 100,0 | 2 µs |

Con prioridad inferior el sensor tiene que esperar a que la carga acabe su ráfaga y llega hasta 27 ms tarde. La siguiente lectura llega antes porque `xTaskDelayUntil()` compensa, y la media se queda en 100 ms. Con prioridad superior el jitter casi desaparece, por eso dejamos la prioridad 4.

Lo que no funcionó / terminado después: todo funcionó en el taller.

## P3 — LED con LEDC

LEDC a 5 kHz y 13 bits en GPIO5. El pulsador cambia entre apagado (0), medio (4096), máximo (8192) y fade de 0 a máximo en 5 s. Con `led <0-100>` se fija el brillo desde la consola. El máximo es 8192 porque ESP-IDF acepta hasta 2^bits y así el LED queda encendido del todo.

La ISR del pulsador solo guarda el instante y lo manda a una cola; la tarea aplica el antirrebote y cambia el LED, porque las funciones de LEDC no se pueden llamar desde una ISR.

Lo que no funcionó / terminado después: al pasar de máximo a fade no se veía el cambio, porque el fade arrancaba sin que el duty llegara a ponerse a 0. Se arregló poniendo el duty a 0 y esperando un tick (`vTaskDelay(1)`) antes de lanzar el fade. Después del taller también se corrigió la condición del ciclo del pulsador, que volvía a apagado sin llegar a fade. La extensión del potenciómetro no se hizo.

## P4 — esp_timer frente a temporizador de FreeRTOS

Una alarma `esp_timer` y un temporizador software de FreeRTOS, los dos a 10 ms. Sus respuestas solo mandan la marca de tiempo a una cola, y la tarea MedidaTimers (núcleo 1) calcula el jitter de cada uno.

| Temporizador | Carga | Medio | Mín | Máx | Jitter | Rebases |
|---|---|---|---|---|---|---|
| esp_timer ([captura](resultados/P4.6_timers_parada.png)) | No | 10,0 | 9,9 | 10,0 | 6 µs | 0 |
| FreeRTOS ([captura](resultados/P4.6_timers_parada.png)) | No | 9,9 | 9,9 | 10,0 | 28 µs | 0 |
| esp_timer ([captura](resultados/P4.6_timers_turnos.png)) | Sí | 10,0 | 9,9 | 10,0 | 6 µs | 0 |
| FreeRTOS ([captura](resultados/P4.6_timers_turnos.png)) | Sí | 10,0 | 0,0 | 31,0 | 31 ms | 11 |

`esp_timer` no nota la carga porque su tarea tiene prioridad 22. El de FreeRTOS lo ejecuta Tmr Svc, con prioridad 1 en el mismo núcleo que las cargas, así que espera a que terminen y luego dispara los atrasados seguidos (por eso el mínimo de 0 ms). No se perdió ningún evento en la cola.

Lo que no funcionó / terminado después: todo funcionó en el taller.

## Fuentes y herramientas

- Guion del taller y documentación de ESP-IDF (LEDC, esp_timer, FreeRTOS).
- API del BME280 de Bosch Sensortec (`components/bme280`).
- Asistente de IA como ayuda para revisar el código y redactar este README.

## Autoevaluación

_Pendiente: rellenar con la rúbrica del guion._

Lo que no llegó a funcionar: en P3, el paso de máximo a fade no se veía hasta añadir una espera de un tick antes del fade.
