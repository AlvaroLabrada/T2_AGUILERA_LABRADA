#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "sdkconfig.h"
#include "led.h"
#include "pines.h"
#include "ejercicio1.h"

static const char *TAG = "led";

#define CONFIG_SEYTR_PERIODO_MS 1000

static led_t s_led;
static volatile bool s_parpadeo_activo = true; /* Si está en true, parpadea; si está en false, obedece a la consola */
static volatile uint32_t s_periodo_ms = CONFIG_SEYTR_PERIODO_MS; /* Variable que almacena el período actual en milisegundos */

static void tarea_led(void *argumento)
{
    (void)argumento;

    TickType_t referencia = xTaskGetTickCount();
    int64_t t_anterior_us = esp_timer_get_time(); /* Para medir el tiempo real entre conmutaciones */

    for (;;) {
        /* Se calcula el semiciclo según el período actual */
        uint32_t semiciclo_ms = s_periodo_ms / 2;
        if (semiciclo_ms == 0) semiciclo_ms = 1;
        const TickType_t intervalo_ticks = pdMS_TO_TICKS(semiciclo_ms);

        /* Usar intervalo_ticks (el nuevo), NO intervalo */
        xTaskDelayUntil(&referencia, intervalo_ticks);

        /* Solo conmuta si el parpadeo no ha sido pausado por la consola */
        if (s_parpadeo_activo) {
            led_conmutar(&s_led);

            /* Traza con el tiempo medido entre conmutaciones: permite comprobar
             * el período real sobre la salida del monitor (criterio C1) */
            const int64_t ahora_us = esp_timer_get_time();
            ESP_LOGI(TAG, "Diferencia=%lld ms, LED=%s",
                     (long long)((ahora_us - t_anterior_us) / 1000),
                     s_led.encendido ? "encendido" : "apagado");
            t_anterior_us = ahora_us;
        }
    }
}

void ejercicio1_cambiar_periodo(uint32_t nuevo_periodo_ms)
{
    if (nuevo_periodo_ms < 20) nuevo_periodo_ms = 20; // Límite mínimo para estabilidad
    s_periodo_ms = nuevo_periodo_ms;
    s_parpadeo_activo = true; // Si estaba pausado, se reanuda con el nuevo período
}

void run_ejercicio1(void)
{
    ESP_ERROR_CHECK(led_iniciar(&s_led, PIN_LED, LED_NIVEL_ENCENDIDO));

    ESP_LOGI(TAG, "TALLER #1 6.1 P1A · Salida digital y parpadeo estable");
    ESP_LOGI(TAG, "#0 encendido t=0.000 ms, origen de la medida");

    if (xTaskCreate(tarea_led, "led", 2048, NULL, 5, NULL) != pdPASS) {
        ESP_LOGE("led", "No se pudo crear la tarea");
    }
}

/* Permite a la consola pausar el parpadeo automático para tomar el control */
void ejercicio1_pausar_parpadeo(bool pausar)
{
    s_parpadeo_activo = !pausar;
}

/* Permite a la consola encender o apagar el LED directamente */
void ejercicio1_fijar_estado_led(bool encender)
{
    s_parpadeo_activo = false; /* Si la consola manda encender/apagar, pausa el parpadeo */
    if (encender) {
        led_encender(&s_led);
    } else {
        led_apagar(&s_led);
    }
}
