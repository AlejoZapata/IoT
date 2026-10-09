/**
 * @file hcsr04.h
 * @brief Interfaz para medir distancias con un sensor ultrasonico HC-SR04.
 */

#include "esp_err.h"
#include <stdint.h>

/** @brief GPIO utilizados para controlar y leer el sensor HC-SR04. */
typedef struct {
  int trig_pin; /**< GPIO de salida conectado a TRIG. */
  int echo_pin; /**< GPIO de entrada conectado a ECHO. */
} hcsr04_config_t;

/**
 * @brief Configura los GPIO TRIG y ECHO del sensor.
 *
 * Inicializa TRIG como salida y ECHO como entrada. La configuracion debe
 * completarse antes de llamar a hcsr04_measure_cm().
 *
 * @param[in] cfg Configuracion de los pines; no puede ser NULL.
 * @return ESP_OK si la configuracion termina. Un error al configurar GPIO se
 *         gestiona mediante ESP_ERROR_CHECK y provoca la terminacion por error.
 */
esp_err_t hcsr04_init(const hcsr04_config_t *cfg);

/**
 * @brief Mide la distancia al objeto mas cercano frente al sensor.
 *
 * Emite un pulso de disparo y calcula la distancia a partir de la duracion del
 * pulso ECHO, usando 0.034 cm/us para la velocidad del sonido.
 *
 * @param[in] cfg Configuracion de pines previamente inicializada; no puede ser NULL.
 * @param[out] out_cm Destino de la distancia en centimetros; no puede ser NULL.
 * @return ESP_OK si se mide el pulso completo o ESP_ERR_TIMEOUT si no se
 *         detecta alguno de sus flancos dentro del tiempo limite.
 */
esp_err_t hcsr04_measure_cm(const hcsr04_config_t *cfg, float *out_cm);