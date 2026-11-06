/* bme280_port.c - Implementación del puerto ESP-IDF para el sensor BME280. */

#include "bme280_port.h"
#include "bme280.h"

#include "driver/i2c_master.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "bme280_port";

typedef struct {
    i2c_master_dev_handle_t handle;
} bme280_i2c_context_t;

static bme280_i2c_context_t s_contexto;
static struct bme280_dev    s_sensor_bme;
static uint32_t             s_espera_us = 0;
static bool                 s_iniciado = false;

static BME280_INTF_RET_TYPE leer_registro(uint8_t registro, uint8_t *datos,
                                          uint32_t cantidad, void *contexto)
{
    bme280_i2c_context_t *i2c = contexto;
    const esp_err_t error = i2c_master_transmit_receive(
        i2c->handle, &registro, sizeof(registro), datos, cantidad, 1000);

    if (error != ESP_OK) {
        ESP_LOGE(TAG, "Error al leer el registro 0x%02X: %s",
                 registro, esp_err_to_name(error));
        return -1;
    }

    return 0;
}

static BME280_INTF_RET_TYPE escribir_registro(uint8_t registro,
                                              const uint8_t *datos,
                                              uint32_t cantidad,
                                              void *contexto)
{
    bme280_i2c_context_t *i2c = contexto;
    uint8_t mensaje[16];

    if (cantidad + 1 > sizeof(mensaje)) {
        ESP_LOGE(TAG, "Bloque demasiado grande al escribir el sensor");
        return -1;
    }

    mensaje[0] = registro;
    for (uint32_t i = 0; i < cantidad; i++) {
        mensaje[i + 1] = datos[i];
    }

    const esp_err_t error = i2c_master_transmit(
        i2c->handle, mensaje, cantidad + 1, 1000);

    if (error != ESP_OK) {
        ESP_LOGE(TAG, "Error al escribir el registro 0x%02X: %s",
                 registro, esp_err_to_name(error));
        return -1;
    }

    return 0;
}

static void esperar_microsegundos(uint32_t tiempo, void *contexto)
{
    (void)contexto;
    esp_rom_delay_us(tiempo);
}

esp_err_t bme280_port_iniciar(gpio_num_t pin_sda, gpio_num_t pin_scl, uint8_t direccion_i2c)
{
    const i2c_master_bus_config_t config = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = pin_sda,
        .scl_io_num = pin_scl,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    i2c_master_bus_handle_t bus;
    esp_err_t err = i2c_new_master_bus(&config, &bus);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Fallo al crear el bus maestro I2C: %s", esp_err_to_name(err));
        return err;
    }

    /* Comprobar si el dispositivo responde en la dirección indicada */
    err = i2c_master_probe(bus, direccion_i2c, 20);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "El sensor no respondio en 0x%02X: %s", direccion_i2c, esp_err_to_name(err));
        return err;
    }
    ESP_LOGI(TAG, "Dispositivo BME280 detectado en 0x%02X", direccion_i2c);

    const i2c_device_config_t sensor_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = direccion_i2c,
        .scl_speed_hz = 100000,
    };

    i2c_master_dev_handle_t sensor;
    err = i2c_master_bus_add_device(bus, &sensor_config, &sensor);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Fallo al asociar el dispositivo al bus I2C: %s", esp_err_to_name(err));
        return err;
    }

    s_contexto.handle = sensor;
    s_sensor_bme.intf = BME280_I2C_INTF;
    s_sensor_bme.intf_ptr = &s_contexto;
    s_sensor_bme.read = leer_registro;
    s_sensor_bme.write = escribir_registro;
    s_sensor_bme.delay_us = esperar_microsegundos;

    int8_t resultado = bme280_init(&s_sensor_bme);
    if (resultado < BME280_OK) {
        ESP_LOGE(TAG, "La API Bosch no pudo inicializar el sensor (error %d)", resultado);
        return ESP_FAIL;
    }
    if (s_sensor_bme.chip_id != BME280_CHIP_ID) {
        ESP_LOGE(TAG, "Chip ID inesperado: 0x%02X (esperado 0x%02X)", s_sensor_bme.chip_id, BME280_CHIP_ID);
        return ESP_ERR_NOT_FOUND;
    }

    ESP_LOGI(TAG, "BME280 confirmado por la API Bosch (ID 0x%02X)", s_sensor_bme.chip_id);

    const struct bme280_settings ajustes = {
        .osr_t = BME280_OVERSAMPLING_1X,
        .osr_p = BME280_OVERSAMPLING_1X,
        .osr_h = BME280_OVERSAMPLING_1X,
        .filter = BME280_FILTER_COEFF_OFF,
        .standby_time = BME280_STANDBY_TIME_0_5_MS,
    };

    resultado = bme280_set_sensor_settings(
        BME280_SEL_OSR_TEMP | BME280_SEL_OSR_PRESS | BME280_SEL_OSR_HUM,
        &ajustes, &s_sensor_bme);
    if (resultado < BME280_OK) {
        ESP_LOGE(TAG, "No se pudieron configurar los sobremuestreos (error %d)", resultado);
        return ESP_FAIL;
    }

    resultado = bme280_cal_meas_delay(&s_espera_us, &ajustes);
    if (resultado < BME280_OK) {
        ESP_LOGE(TAG, "No se pudo calcular el tiempo de medida (error %d)", resultado);
        return ESP_FAIL;
    }

    s_iniciado = true;
    ESP_LOGI(TAG, "Sensor BME280 listo (espera conversion: %lu us)", (unsigned long)s_espera_us);
    return ESP_OK;
}

esp_err_t bme280_port_leer(bme280_datos_t *datos)
{
    if (!s_iniciado || datos == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    int8_t estado = bme280_set_sensor_mode(BME280_POWERMODE_FORCED, &s_sensor_bme);
    if (estado < BME280_OK) {
        ESP_LOGE(TAG, "Fallo al poner en modo forzado (error %d)", estado);
        return ESP_FAIL;
    }

    /* Esperar a que termine la conversión */
    vTaskDelay(pdMS_TO_TICKS(s_espera_us / 1000 + 5));

    struct bme280_data medida;
    estado = bme280_get_sensor_data(BME280_ALL, &medida, &s_sensor_bme);
    if (estado < BME280_OK) {
        ESP_LOGE(TAG, "Fallo al leer datos del sensor (error %d)", estado);
        return ESP_FAIL;
    }

    datos->temperatura = (float)medida.temperature;
    datos->presion     = (float)medida.pressure;
    datos->humedad     = (float)medida.humidity;

    return ESP_OK;
}

