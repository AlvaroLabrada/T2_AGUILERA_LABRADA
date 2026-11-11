# Taller 2 — FreeRTOS: tareas, prioridades y temporización

**Sistemas Empotrados y de Tiempo Real (SEyTR 2026-27)**  
Máster Universitario en Robótica y Automática  
Escuela Técnica Superior de Ingeniería Industrial · Ciudad Real  
Universidad de Castilla-La Mancha

---

## 1. Integrantes del equipo

| Integrante | Rol |
|---|---|
| **Heysser Aguilera** | Desarrollo conjunto y depuración |
| **Álvaro Labrada Isidro** | Desarrollo conjunto y depuración |

El trabajo y la preparación del taller se realizan de forma conjunta y coordinada.

---

## 2. Tabla de asignación de pines

| Elemento | GPIO / Puerto | Dirección | Descripción y niveles lógicos |
|---|:---:|:---:|---|
| **LED externo** | `GPIO5` | Salida | Con resistencia limitadora en serie a masa. Nivel que enciende: **`1`**. |
| **Pulsador** | `GPIO4` | Entrada | Pulsador conectado a masa con **resistencia pull-up externa** a 3V3. Pull-up interna desactivada. Nivel pulsado: **`0`**. |
| **I2C — SDA** | `GPIO8` | Bidireccional | Línea de datos del bus I2C del sensor BME280 (pull-up incorporada en la placa del sensor). |
| **I2C — SCL** | `GPIO9` | Salida | Línea de reloj del bus I2C (100 kHz maestro). |
| **Consola** | `UART0` | E/S serie | Conector **USB-to-UART** de la ESP32-S3-DevKitC-1 a 115200 baudios (8N1). |
| **Potenciómetro (Extensión P3)** | `GPIO1` | Entrada analógica | Canal ADC1_CH0 para control analógico del brillo por modulación LEDC. |

> **Principio de diseño:** El fichero [`main/pines.h`](main/pines.h) es la **única fuente de verdad** del proyecto donde se configuran los pines físicos. Ningún componente reutilizable hardcodea números de GPIO.

---

## 3. Decisiones tomadas de antemano (Capítulo 5)

### 3.1. Periodo de muestreo del sensor BME280
- **Periodo fijado:** **2000 ms (2,0 s)**.
- **Justificación técnica:**
  1. **Tiempo de conversión:** En modo forzado con sobremuestreo $1\times$ en temperatura, presión y humedad, el tiempo de medición que calcula la API oficial de Bosch (`bme280_cal_meas_delay`) es de aproximadamente **9,3 ms**.
  2. **Dinámica de las magnitudes:** La temperatura, la humedad y la presión atmosférica en interiores son magnitudes físicas de variación lenta cuyas constantes de tiempo se miden en decenas de segundos o minutos. Un periodo de 2 s cumple con holgura el teorema de muestreo para capturar las variaciones reales sin aliasing.
  3. **Eficiencia y recursos:** Muestrear cada 2 segundos evita el autocalentamiento del sensor (que falsearía las lecturas térmicas), minimiza el consumo energético de la placa y no sobrecarga el bus I2C ni la CPU, permitiendo a FreeRTOS planificar el resto de tareas de usuario y del sistema con mínima latencia. En la Práctica 2 se medirá la regularidad y desviación de este intervalo con `xTaskDelayUntil()`.

### 3.2. Extensión de la Práctica 3 (LEDC y ADC)
- Se reserva el **`GPIO1` (ADC1 Canal 0)** para la lectura del potenciómetro de U2-S4 que modulará el ciclo de trabajo del LED mediante el periférico LEDC.

---

## 4. Estructura del proyecto

El firmware sigue la estructura modular requerida para el Taller 2:

```text
T2_AGUILERA_LABRADA/
├── CMakeLists.txt              # Configuración CMake del proyecto (T2_AGUILERA_LABRADA)
├── sdkconfig.defaults          # Configuración base de FreeRTOS e inspección de tareas
├── .gitignore                  # Excluye build/, sdkconfig, .vscode/, .devcontainer/ y .clangd
├── README.md                   # Documentación, integrantes, pines y justificaciones
├── main/
│   ├── CMakeLists.txt          # Registro del componente principal
│   ├── main.c                  # Inicialización y verificación de lectura del sensor
│   └── pines.h                 # Definición única de pines GPIO y dirección I2C
└── components/
    ├── led/                    # Control del LED en pin digital (encender/apagar/conmutar)
    ├── pulsador/               # Entrada digital del pulsador y filtro antirrebote (filtro.c/h)
    ├── bme280/                 # API Bosch Sensortec + adaptador de bus I2C (bme280_port.c/h)
    └── consola/                # Consola serie interactiva por UART0 / STDIN con tabla de comandos
```

### Componentes reutilizables en `components/`:
- **`led`**: Abstracción del LED digital. Desacoplado del número de pin y del montaje físico.
- **`pulsador`**: Abstracción de la entrada digital con interrupciones por flanco y módulo de **antirrebote** (`filtro_flanco`) con tiempo de guarda y confirmación de nivel.
- **`bme280`**: Librería oficial de Bosch Sensortec combinada con el puerto I2C maestro para ESP-IDF (`bme280_port`), permitiendo inicializar y leer el sensor de forma limpia.
- **`consola`**: Tarea de FreeRTOS que recibe comandos por UART0/STDIN, valida longitudes de línea (máx. 64 caracteres) y despacha acciones de forma modular.

---

## 5. Configuración de FreeRTOS (`sdkconfig.defaults`)

El archivo [`sdkconfig.defaults`](sdkconfig.defaults) incluye las directivas requeridas para la inspección y depuración de tareas en el Taller 2:

```ini
CONFIG_IDF_TARGET="esp32s3"
CONFIG_FREERTOS_HZ=1000
CONFIG_FREERTOS_USE_TRACE_FACILITY=y
CONFIG_FREERTOS_USE_STATS_FORMATTING_FUNCTIONS=y
CONFIG_FREERTOS_GENERATE_RUN_TIME_STATS=y
CONFIG_FREERTOS_VTASKLIST_INCLUDE_COREID=y
```

Estas directivas activan las funciones del kernel de FreeRTOS para generar estadísticas de ejecución en tiempo real (`vTaskGetRunTimeStats`), listar tareas con su afinidad de núcleo (`vTaskList`) y obtener el reparto de carga entre núcleos.

---

## 6. Compilación, grabación y ejecución

El proyecto se desarrolla con la extensión oficial **ESP-IDF para VS Code** (o línea de comandos `idf.py`):

1. **Seleccionar destino (Target):**
   ```bash
   idf.py set-target esp32s3
   ```
2. **Compilar el proyecto:**
   ```bash
   idf.py build
   ```
3. **Flashear y abrir monitor serie (conectar por puerto USB-to-UART):**
   ```bash
   idf.py -p /dev/tty.usbserial-* flash monitor
   ```

Al arrancar, `app_main()` inicializa el bus I2C, detecta el sensor BME280 en `0x76`, calibra el dispositivo y emite por el monitor serie una primera lectura de confirmación de temperatura, presión y humedad.

6.2 P2 
Si la prioridad del sensor es inferior al resto de tareas se muestra en las imagenes:
"Prioridad inferior parada.png"
"Prioridad inferior turnos.png"
En este caso cuando el sensor termine su tiempo de espera, el planificador no le dejará entrar si una tarea de carfa está en medio de sus 20ms de cálculo.


Si la prioridad del sensor es superior al resto de tareas se muestra en las imagenes:
"Prioridad superior parada.png"
"Prioridad superior turnos.png"
En este caso cuando el sensor termine su tiempo de espera, el planificador le deja entrar, ya que su prioridad es superior a cualquier otra tarea.

---

## 6.3 P3 — Salida modulada con LEDC

**Frecuencia: 5 kHz.** El ojo no detecta parpadeo por encima de ~100 Hz. Con el reloj LEDC a 80 MHz caben `80.000.000 / 5000 = 16.000` pasos por periodo, lo que permite 13 bits. 14 bits necesitarían 16.384 y no caben.

**Resolución: 13 bits.** Da 8192 niveles de brillo, suficiente para transiciones visualmente suaves.

### Modos

| Modo | Duty |
|---|:---:|
| `APAGADO` | 0 |
| `MEDIO` (50 %) | 4096 |
| `MAXIMO` (100 %) | 8192 |
| `FADE` (0 → 8192 en 2 s por hardware) | — |

### Por qué el duty se cambia en la tarea y no en la ISR

Las llamadas a LEDC no son seguras desde una ISR: pueden hacer esperas internas. La ISR solo anota el instante con `esp_timer_get_time()` y lo manda a la tarea por una cola. La tarea aplica el antirrebote con `filtro_flanco` y, si la pulsación es válida, cambia el modo.
