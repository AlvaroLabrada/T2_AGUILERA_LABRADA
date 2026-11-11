#include "ejercicio3.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "consola.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "filtro.h"
#include "pines.h"
#include "pulsador.h"

// 5000 hz está por encima los 20hz que pide el humano, por lo que no lo
// notaremos
#define LEDC_FRECUENCIA 5000

// 13 bits nos da un margen de 8192 niveles para jugar con el brillo
#define LEDC_RESOLUCION LEDC_TIMER_13_BIT

#define DUTY_MAX 8192
#define DUTY_MEDIO 4096

typedef enum { APAGADO, MEDIO, MAXIMO, FADE } modo_led_t;
const char *nombres_modos[] = {"APAGADO", "MEDIO", "MAXIMO", "FADE"};

static modo_led_t modo_actual = APAGADO;

static pulsador_t pulsador;
static QueueHandle_t cola_pulsador;

// Función que cambia el modo del led
static void cambiar_modo_led(modo_led_t modo) {
  ledc_fade_stop(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
  ESP_LOGI("P3", "Modo actual : %s", nombres_modos[modo]);

  switch (modo) {
  case APAGADO:
    ledc_set_duty_and_update(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0, 0);
    break;
  case MEDIO:
    ledc_set_duty_and_update(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 4096, 0);
    break;
  case MAXIMO:
    ledc_set_duty_and_update(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 8192, 0);
    break;
  case FADE:

    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    vTaskDelay(1);
    ledc_set_fade_time_and_start(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 8192,
                                 5000, LEDC_FADE_NO_WAIT);
    break;
  }
}

static void IRAM_ATTR isr_pulsador(void *arg) {
  int64_t us = esp_timer_get_time();
  BaseType_t despertar = pdFALSE;
  xQueueSendFromISR(cola_pulsador, &us, &despertar);
  if (despertar) {
    portYIELD_FROM_ISR();
  }
}

static bool sigue_pulsado(int64_t instante_us, void *contexto) {
  int64_t falta_us = instante_us - esp_timer_get_time();
  if (falta_us > 0) {
    vTaskDelay(pdMS_TO_TICKS(falta_us / 1000) + 1);
  }
  return pulsador_pulsado(&pulsador);
}

// Tarea que se encarga de detectar las pulsaciones
static void tarea_pulsador(void *args) {
  int64_t us_evento;
  filtro_t filtro = {
      .guarda_us = 300000,
      .confirmar = true,
      .confirmacion_us = 20000,
  };
  filtro_reiniciar(&filtro);

  while (1) {
    if (xQueueReceive(cola_pulsador, &us_evento, portMAX_DELAY) == pdTRUE) {
      char marca = filtro_flanco(&filtro, us_evento, sigue_pulsado, NULL);
      if (marca == MARCA_PULSACION) {
        modo_actual++;
        if (modo_actual >= FADE) {
          modo_actual = APAGADO;
        }
        cambiar_modo_led(modo_actual);
      }
    }
  }
}

void run_ejercicio3(void) {

  // Configuración del temporizador
  const ledc_timer_config_t temporizador = {
      .speed_mode = LEDC_LOW_SPEED_MODE,
      .duty_resolution = LEDC_RESOLUCION,
      .timer_num = LEDC_TIMER_0,
      .freq_hz = LEDC_FRECUENCIA,
      .clk_cfg = LEDC_AUTO_CLK,
  };

  // Aplicamos la configuración del temporizador
  ESP_ERROR_CHECK(ledc_timer_config(&temporizador));

  // Configuración del canal
  const ledc_channel_config_t canal = {
      .gpio_num = PIN_LED,
      .speed_mode = LEDC_LOW_SPEED_MODE,
      .channel = LEDC_CHANNEL_0,
      .timer_sel = LEDC_TIMER_0,
  };

  // Instalamos la función de fade
  ledc_fade_func_install(0);

  // Aplicamos la configuración del canal
  ESP_ERROR_CHECK(ledc_channel_config(&canal));

  // Iniciamos el pulsador
  ESP_ERROR_CHECK(
      pulsador_iniciar(&pulsador, PIN_PULSADOR, PULSADOR_NIVEL_PULSADO));

  // Creamos la cola
  cola_pulsador = xQueueCreate(10, sizeof(int64_t));
  gpio_install_isr_service(0);
  gpio_isr_handler_add(PIN_PULSADOR, isr_pulsador, NULL);

  // Iniciamos la tarea
  xTaskCreatePinnedToCore(tarea_pulsador, "Pulsador", 3072, NULL, 3, NULL, 0);

  // Estado inicial
  modo_actual = APAGADO;
  cambiar_modo_led(modo_actual);
}
