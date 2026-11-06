#pragma once
#include "driver/gpio.h"
#define PIN_LED              GPIO_NUM_5 /* Salida digital: LED con su resistencia y masa */
#define PIN_PULSADOR         GPIO_NUM_4 /* Entrada: pulsador a masa, pull-up externa */
#define PIN_I2C_SDA          GPIO_NUM_8 /* Bus I2C: datos */
#define PIN_I2C_SCL          GPIO_NUM_9 /* Bus I2C: reloj */
#define LED_NIVEL_ENCENDIDO  1
#define PULSADOR_NIVEL_PULSADO  0
#define BME280_I2C_ADDR  0x76
