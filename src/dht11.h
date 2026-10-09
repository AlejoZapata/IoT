/**
 * @file dht11.h
 * @brief Interfaz para inicializar y leer un sensor de temperatura y humedad DHT11.
 */

#include <stdint.h>
#include "esp_err.h"

/** @brief Lecturas de temperatura y humedad proporcionadas por el DHT11. */
typedef struct {
    float temperature; /**< Temperatura en grados Celsius. */
    float humidity;    /**< Humedad relativa en porcentaje. */
} dht11_data_t;

/**
 * @brief Prepara el GPIO para comunicarse con el sensor DHT11.
 *
 * Configura el pin como salida con nivel alto y habilita el pull-up interno.
 * Debe llamarse antes de dht11_read().
 *
 * @param gpio_pin GPIO conectado a la linea de datos del sensor.
 * @return ESP_OK al completar la configuracion.
 */
esp_err_t dht11_init(int gpio_pin);

/**
 * @brief Solicita y obtiene una muestra del sensor DHT11.
 *
 * La funcion bloquea mientras genera la senal de inicio y mide la respuesta.
 * La muestra solo se copia a @p out si la comunicacion y el checksum son validos.
 *
 * @param gpio_pin GPIO conectado a la linea de datos del sensor.
 * @param[out] out Estructura donde se guardaran temperatura y humedad; no puede ser NULL.
 * @return ESP_OK si la muestra es valida, ESP_ERR_TIMEOUT si no se recibe una
 *         transicion esperada, o ESP_ERR_INVALID_CRC si falla el checksum.
 */
esp_err_t dht11_read(int gpio_pin, dht11_data_t *out);
