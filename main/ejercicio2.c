#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"

#include "pulsador.h"
#include "filtro.h"
#include "pines.h"
#include "ejercicio2.h"

static const char *TAG = "pulsador";

#define GUARDA_MS           50     /*Los milisegundos de la ventana de guarda para el antirrebote.*/
#define CONFIRMACION_MS     20     /*Los milisegundos que espera antes de leer si el botón sigue pulsado.*/
#define LONGITUD_COLA       10     /*Longitud de la cola de eventos.*/

/*Paquete de datos mínimo que la ISR enviará a la tarea*/
typedef struct {
    uint32_t pin;
    int64_t  us;
} evento_t;

/* La cola de eventos se crea al arrancar, antes de instalar la interrupción,
 * con su longitud y el tamaño del elemento. */
static QueueHandle_t s_cola;
static pulsador_t    s_pulsador;

/*Implementación para la consola*/
static uint32_t s_total_pulsaciones = 0;
uint32_t ejercicio2_obtener_pulsaciones(void)
{
    return s_total_pulsaciones;
}

/*Interrupción que captura los microsegundos y envia por cola sin trazas ni esperas*/
static void IRAM_ATTR isr_cola(void *arg) 
{
    evento_t e = { .pin = (uint32_t)arg,
                   .us = esp_timer_get_time() };
    BaseType_t hp = pdFALSE;
    xQueueSendFromISR(s_cola, &e, &hp); /* meter un dato (e) dentro de una cola desde dentro de una interrupción */
    if (hp) portYIELD_FROM_ISR(); /*Si hay una tarea de mayor prioridad se cambia a true solo*/
}

/*Esperar a que acabe el rebote y comprobar que sigue pulsado*/
static bool sigue_pulsado(int64_t instante_us, void *contexto)
{
    const pulsador_t *pulsador = contexto;
    const int64_t falta_us = instante_us - esp_timer_get_time();
    if (falta_us > 0) {
        vTaskDelay(pdMS_TO_TICKS((uint32_t)((falta_us + 999) / 1000)) + 1);
    }
    return pulsador_pulsado(pulsador);
}

static void tarea_entrada(void *argumento)
{
    (void)argumento;
    evento_t e;
    //uint32_t total = 0;

    filtro_t filtro = {
        .guarda_us       = (int64_t)GUARDA_MS * 1000,
        .confirmacion_us = (int64_t)CONFIRMACION_MS * 1000,
        .confirmar       = true,
    };
    filtro_reiniciar(&filtro);

    for (;;) {
        if (xQueueReceive(s_cola, &e, portMAX_DELAY) == pdTRUE) {
            const char marca = filtro_flanco(&filtro, e.us, sigue_pulsado, &s_pulsador);
            if (marca == MARCA_PULSACION) {
                s_total_pulsaciones++;
                ESP_LOGI(TAG, "Pulsacion valida: %" PRIu32, s_total_pulsaciones);
            }
        }
    }
}


void run_ejercicio2(void)
{
    ESP_LOGI(TAG, "SEyTR - Pulsador por interrupcion con filtro antirrebote");

    /*Iniciar el componente del pulsador con su pin y lógica física */
    ESP_ERROR_CHECK(pulsador_iniciar(&s_pulsador, PIN_PULSADOR, PULSADOR_NIVEL_PULSADO));

    /*Desactivar resistencia interna: el reposo lo fija la resistencia externa en protoboard */
    ESP_ERROR_CHECK(pulsador_polarizar(&s_pulsador, false));

    /*Crear la cola antes de activar la interrupción para que no reciba eventos sobre la nada */
    s_cola = xQueueCreate(LONGITUD_COLA, sizeof(evento_t));
    if (s_cola == NULL) {
        ESP_LOGE(TAG, "Error: no hay memoria para crear la cola");
        return;
    }

    /* El servicio de interrupciones de GPIO se instala una sola vez para todo
     * el proyecto: diapositiva 12. Con 0, sin ESP_INTR_FLAG_IRAM. */    
     ESP_ERROR_CHECK(gpio_install_isr_service(0));

    /*Registrar el manejador (ISR) en el pin del pulsador*/
    ESP_ERROR_CHECK(gpio_isr_handler_add(PIN_PULSADOR, isr_cola, (void *)PIN_PULSADOR));

    /*Crear la tarea que consumirá los eventos y aplicará el filtro (prioridad 10)*/
    const BaseType_t entrada = xTaskCreate(tarea_entrada, "entrada", 4096, NULL, 10, NULL);
    if (entrada != pdPASS) {
        ESP_LOGE(TAG, "no hay memoria para la tarea de entrada");
        return;
    }

}