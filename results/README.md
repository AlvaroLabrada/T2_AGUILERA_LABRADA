# Resultados (evidencias)

Capturas y extractos de traza del taller. Cada archivo se cita desde el apartado
de su práctica en el [README principal](../README.md).

| Archivo | Qué debe contener | Cómo obtenerlo |
|---|---|---|
| `01_parpadeo.txt` | Al menos **diez conmutaciones** seguidas del LED con el período medido. | Copiar del monitor las líneas `led: Diferencia=NNN ms, LED=...` (deben rondar los 500 ms). |
| `02_pulsaciones.txt` | Recuento hasta **diez pulsaciones** (tres manteniendo el pulsador ~1 s). | Copiar las líneas `main: Pulsacion valida: N` hasta `N=10`. |
| `03_sensor.txt` | **Identificación** del sensor + una **lectura en reposo** + otra **ante un cambio** (mano encima). | Copiar `bme280: BME280 confirmado ... ID 0x60` del arranque y dos salidas del comando `e` (estado), una normal y otra con la mano sobre el sensor. |
| `04_consola.txt` | Sesión de consola con los **tres comandos**, un comando inexistente, una **línea vacía** y una **línea muy larga**. | Copiar del monitor una sesión con `h`, `e`, `l 500`, un comando que no exista, un Intro en vacío y una línea que supere el límite. |

Formato libre: valen archivos de texto con el extracto del monitor o capturas de
pantalla (`.png`). Si usáis capturas, nombradlas igual (`01_parpadeo.png`, etc.).
