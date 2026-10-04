/**
 * @file main.c
 * @brief Sistema de control para ESP32-S3 usando ESP-IDF y FreeRTOS.
 * @details Este archivo gestiona la lectura de múltiples sensores de temperatura (dos LM35 vía ADC y un DHT11)
 *          y el control de 5 relés por medio de comandos enviados por consola serial.
 * @author Alejandro zapata - Miguel alvarez - Thomas Ciro / Proyecto
 * @date 2026-10-04  
 * w
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "driver/gpio.h"

#include "esp_log.h"
#include "esp_err.h"
#include "esp_rom_sys.h"

#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

// ============================================================
// DEFINICIONES Y CONSTANTES
// ============================================================

/** @brief Tag principal para los mensajes de registro (ESP_LOG). */
#define TAG "CONTROL"

/** @name Pines GPIO para Relés
 *  @{
 */
#define RELAY_1_GPIO    GPIO_NUM_35  /**< GPIO del Relé 1 */
#define RELAY_2_GPIO    GPIO_NUM_36  /**< GPIO del Relé 2 */
#define RELAY_3_GPIO    GPIO_NUM_37  /**< GPIO del Relé 3 */
#define RELAY_4_GPIO    GPIO_NUM_38  /**< GPIO del Relé 4 */
#define RELAY_5_GPIO    GPIO_NUM_39  /**< GPIO del Relé 5 */
/** @} */

/** @brief Cantidad total de relés en el sistema. */
#define NUM_RELAYS      5

/** @brief Estado lógico para activar relés (Activo en LOW). */
#define RELAY_ON        0
/** @brief Estado lógico para desactivar relés. */
#define RELAY_OFF       1

/** @name Configuración Sensor LM35 #1
 *  @{
 */
#define LM35_GPIO           GPIO_NUM_9      /**< GPIO del sensor LM35 #1 */
#define LM35_ADC_CHANNEL    ADC_CHANNEL_8   /**< Canal ADC1 asignado al LM35 #1 */
#define LM35_INTERVAL_MS    1000            /**< Intervalo de lectura en ms */
/** @} */

/** @name Configuración Sensor LM35 #2
 *  @{
 */
#define LM35_2_GPIO         GPIO_NUM_11     /**< GPIO del sensor LM35 #2 (ESP32-S3) */
#define LM35_2_ADC_CHANNEL  ADC_CHANNEL_0   /**< Canal ADC2 asignado al LM35 #2 */
/** @} */

/** @name Configuración Sensor DHT11
 *  @{
 */
#define DHT11_GPIO          GPIO_NUM_40     /**< GPIO de datos para el DHT11 */
#define DHT11_INTERVAL_MS   2000            /**< Intervalo de lectura recomendado (2s) */
/** @} */

// ============================================================
// ESTRUCTURAS Y TIPOS
// ============================================================

/**
 * @struct relay_command_t
 * @brief Estructura enviada a la cola de FreeRTOS para conmutar un relé.
 */
typedef struct {
    uint8_t relay; /**< Número de relé objetivo (1 a 5) */
} relay_command_t;

// ============================================================
// VARIABLES ESTÁTICAS / GLOBALES
// ============================================================

/** @brief Cola FreeRTOS para la recepción de comandos de relés. */
static QueueHandle_t relay_queue;

/** @brief Mapeo de pines GPIO correspondientes a cada relé. */
static const gpio_num_t relay_gpios[NUM_RELAYS] = {
    RELAY_1_GPIO,
    RELAY_2_GPIO,
    RELAY_3_GPIO,
    RELAY_4_GPIO,
    RELAY_5_GPIO
};

/** @brief Arreglo con el estado actual de cada relé (true: encendido, false: apagado). */
static bool relay_state[NUM_RELAYS] = {false, false, false, false, false};

/** @brief Manejador de la unidad ADC1 en modo oneshot. */
static adc_oneshot_unit_handle_t adc1_handle;
/** @brief Manejador de la calibración para ADC1. */
static adc_cali_handle_t adc1_cali_handle = NULL;
/** @brief Bandera que indica si la calibración de ADC1 se inicializó con éxito. */
static bool adc1_calibration_enabled = false;

/** @brief Manejador de la unidad ADC2 en modo oneshot. */
static adc_oneshot_unit_handle_t adc2_handle;
/** @brief Manejador de la calibración para ADC2. */
static adc_cali_handle_t adc2_cali_handle = NULL;
/** @brief Bandera que indica si la calibración de ADC2 se inicializó con éxito. */
static bool adc2_calibration_enabled = false;

// ============================================================
// FUNCIONES DE INICIALIZACIÓN
// ============================================================

/**
 * @brief Configura los pines GPIO asignados a los relés como salidas.
 * @note Inicializa todos los relés en estado apagado (`RELAY_OFF`).
 */
static void relay_gpio_init(void) {
    for (int i = 0; i < NUM_RELAYS; i++) {
        gpio_config_t io_conf = {
            .pin_bit_mask = (1ULL << relay_gpios[i]),
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE
        };
        ESP_ERROR_CHECK(gpio_config(&io_conf));
        gpio_set_level(relay_gpios[i], RELAY_OFF);
    }
}

/**
 * @brief Inicializa la unidad ADC1 y el esquema de calibración para el LM35 #1.
 */
static void lm35_adc1_init(void) {
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc1_handle));

    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, LM35_ADC_CHANNEL, &config));

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    adc_cali_curve_fitting_config_t cali_config = {
        .unit_id = ADC_UNIT_1,
        .chan = LM35_ADC_CHANNEL,
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT
    };

    esp_err_t ret = adc_cali_create_scheme_curve_fitting(&cali_config, &adc1_cali_handle);
    if (ret == ESP_OK) {
        adc1_calibration_enabled = true;
        ESP_LOGI(TAG, "LM35 #1: calibracion ADC habilitada");
    } else {
        ESP_LOGW(TAG, "LM35 #1: sin calibracion ADC");
    }
#endif

    ESP_LOGI(TAG, "LM35 #1 -> GPIO %d / ADC1_CHANNEL8", LM35_GPIO);
}

/**
 * @brief Inicializa la unidad ADC2 y el esquema de calibración para el LM35 #2.
 */
static void lm35_adc2_init(void) {
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_2
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc2_handle));

    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc2_handle, LM35_2_ADC_CHANNEL, &config));

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    adc_cali_curve_fitting_config_t cali_config = {
        .unit_id = ADC_UNIT_2,
        .chan = LM35_2_ADC_CHANNEL,
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT
    };

    esp_err_t ret = adc_cali_create_scheme_curve_fitting(&cali_config, &adc2_cali_handle);
    if (ret == ESP_OK) {
        adc2_calibration_enabled = true;
        ESP_LOGI(TAG, "LM35 #2: calibracion ADC habilitada");
    } else {
        ESP_LOGW(TAG, "LM35 #2: sin calibracion ADC");
    }
#endif

    ESP_LOGI(TAG, "LM35 #2 -> GPIO %d / ADC2_CHANNEL0", LM35_2_GPIO);
}

/**
 * @brief Inicializa el pin GPIO asignado al sensor DHT11 en modo entrada con Pull-Up.
 */
static void dht11_init(void) {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << DHT11_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));
    ESP_LOGI(TAG, "DHT11 -> GPIO %d", DHT11_GPIO);
}

// ============================================================
// FUNCIONES DE CONTROL DE RELÉS
// ============================================================

/**
 * @brief Establece el estado de un relé específico.
 * 
 * @param relay Número del relé a controlar (1 a 5).
 * @param state Estado deseado (`true` para encender, `false` para apagar).
 */
static void relay_set(uint8_t relay, bool state) {
    if (relay < 1 || relay > NUM_RELAYS) {
        return;
    }

    uint8_t index = relay - 1;
    relay_state[index] = state;
    gpio_set_level(relay_gpios[index], state ? RELAY_ON : RELAY_OFF);

    ESP_LOGI(TAG, "RELE %d -> %s", relay, state ? "ON" : "OFF");
}

/**
 * @brief Imprime en la consola de logs el estado actual de todos los relés.
 */
static void relay_print_status(void) {
    ESP_LOGI(TAG, "-----------------------------");
    for (int i = 0; i < NUM_RELAYS; i++) {
        ESP_LOGI(TAG, "RELE %d: %s", i + 1, relay_state[i] ? "ON" : "OFF");
    }
    ESP_LOGI(TAG, "-----------------------------");
}

// ============================================================
// TAREAS FREERTOS
// ============================================================

/**
 * @brief Tarea FreeRTOS encargada de la lógica de conmutación de relés.
 * @details Escucha la cola `relay_queue`. Implementa un interbloqueo mutuo entre el Relé 2 y el Relé 3
 *          (si se enciende uno, apaga automáticamente el otro).
 * @param pvParameters Parámetros de la tarea (no utilizado).
 */
static void relay_task(void *pvParameters) {
    relay_command_t command;

    while (1) {
        if (xQueueReceive(relay_queue, &command, portMAX_DELAY) == pdTRUE) {
            uint8_t relay = command.relay;

            if (relay < 1 || relay > NUM_RELAYS) {
                continue;
            }

            // Lógica con exclusión para Relé 2 y Relé 3
            if (relay == 2) {
                if (!relay_state[1]) {
                    relay_set(3, false); // Apagar relé 3
                    relay_set(2, true);  // Encender relé 2
                } else {
                    relay_set(2, false); // Apagar relé 2
                }
            } else if (relay == 3) {
                if (!relay_state[2]) {
                    relay_set(2, false); // Apagar relé 2
                    relay_set(3, true);  // Encender relé 3
                } else {
                    relay_set(3, false); // Apagar relé 3
                }
            } else {
                // Relés 1, 4 y 5: Conmutación normal (toggle)
                relay_set(relay, !relay_state[relay - 1]);
            }

            relay_print_status();
        }
    }
}

/**
 * @brief Tarea FreeRTOS para procesar la entrada serial del usuario.
 * @details Interpreta comandos que comienzan con ':' seguido de un número del 1 al 5 (ej. `:1`)
 *          y encola la instrucción para `relay_task`.
 * @param pvParameters Parámetros de la tarea (no utilizado).
 */
static void serial_task(void *pvParameters) {
    char command[32];
    int index = 0;

    while (1) {
        int c = getchar();

        if (c == EOF) {
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        if (c == '\n' || c == '\r') {
            if (index == 0) {
                continue;
            }

            command[index] = '\0';
            ESP_LOGI(TAG, "Comando recibido: %s", command);

            if (command[0] == ':' && index >= 2) {
                int relay = atoi(&command[1]);

                if (relay >= 1 && relay <= 5) {
                    relay_command_t relay_command = {
                        .relay = (uint8_t)relay
                    };

                    if (xQueueSend(relay_queue, &relay_command, pdMS_TO_TICKS(100)) != pdTRUE) {
                        ESP_LOGW(TAG, "Cola de reles llena");
                    }
                } else {
                    ESP_LOGW(TAG, "Rele invalido");
                }
            } else {
                ESP_LOGW(TAG, "Use :1 hasta :5");
            }

            index = 0;
        } else {
            if (index < sizeof(command) - 1) {
                command[index++] = (char)c;
            } else {
                index = 0;
                ESP_LOGW(TAG, "Comando demasiado largo");
            }
        }
    }
}

/**
 * @brief Tarea FreeRTOS para la lectura periódica de los dos sensores LM35.
 * @details Lee los valores raw del ADC, aplica conversión o calibración a mV y calcula
 *          la temperatura basada en la escala del sensor (10 mV/°C).
 * @param pvParameters Parámetros de la tarea (no utilizado).
 */
static void lm35_task(void *pvParameters) {
    int raw1, raw2;
    int voltage1_mv, voltage2_mv;

    while (1) {
        esp_err_t result1 = adc_oneshot_read(adc1_handle, LM35_ADC_CHANNEL, &raw1);
        esp_err_t result2 = adc_oneshot_read(adc2_handle, LM35_2_ADC_CHANNEL, &raw2);

        // --- Procesar LM35 #1 ---
        if (result1 == ESP_OK) {
            if (adc1_calibration_enabled) {
                adc_cali_raw_to_voltage(adc1_cali_handle, raw1, &voltage1_mv);
            } else {
                voltage1_mv = (raw1 * 3300) / 4095;
            }
            float temperature1 = voltage1_mv / 10.0f;
            ESP_LOGI(TAG, "LM35 #1 -> ADC: %d | %d mV | %.2f C", raw1, voltage1_mv, temperature1);
        } else {
            ESP_LOGE(TAG, "Error leyendo LM35 #1");
        }

        // --- Procesar LM35 #2 ---
        if (result2 == ESP_OK) {
            if (adc2_calibration_enabled) {
                adc_cali_raw_to_voltage(adc2_cali_handle, raw2, &voltage2_mv);
            } else {
                voltage2_mv = (raw2 * 3300) / 4095;
            }
            float temperature2 = voltage2_mv / 10.0f;
            ESP_LOGI(TAG, "LM35 #2 -> ADC: %d | %d mV | %.2f C", raw2, voltage2_mv, temperature2);
        } else {
            ESP_LOGE(TAG, "Error leyendo LM35 #2");
        }

        vTaskDelay(pdMS_TO_TICKS(LM35_INTERVAL_MS));
    }
}

// ============================================================
// FUNCIONES AUXILIARES SENSOR DHT11
// ============================================================

/**
 * @brief Espera hasta que el pin del DHT11 alcance un nivel lógico específico o expire el tiempo.
 * 
 * @param level Nivel lógico esperado (0 o 1).
 * @param timeout_us Tiempo límite de espera en microsegundos.
 * @return `true` si el nivel cambió antes del timeout, `false` si expiró el tiempo.
 */
static bool dht11_wait_for_level(int level, uint32_t timeout_us) {
    uint32_t elapsed = 0;
    while (gpio_get_level(DHT11_GPIO) != level) {
        esp_rom_delay_us(1);
        elapsed++;
        if (elapsed >= timeout_us) {
            return false;
        }
    }
    return true;
}

/**
 * @brief Lee los datos de humedad y temperatura del sensor DHT11 usando protocolo One-Wire por software.
 * 
 * @param[out] humidity Puntero donde se almacenará el valor de la humedad relativa (%).
 * @param[out] temperature Puntero donde se almacenará el valor de la temperatura (°C).
 * @return `true` si la lectura y el checksum fueron exitosos, `false` en caso de error.
 */
static bool dht11_read(float *humidity, float *temperature) {
    uint8_t data[5] = {0, 0, 0, 0, 0};

    // Señal de inicio (Start Signal)
    gpio_set_direction(DHT11_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(DHT11_GPIO, 0);
    vTaskDelay(pdMS_TO_TICKS(20)); // Mantener LOW durante ~18-20 ms

    gpio_set_level(DHT11_GPIO, 1);
    esp_rom_delay_us(30);

    // Liberar línea para recibir datos
    gpio_set_direction(DHT11_GPIO, GPIO_MODE_INPUT);

    // Respuesta del sensor (ACK)
    if (!dht11_wait_for_level(0, 100) || 
        !dht11_wait_for_level(1, 100) || 
        !dht11_wait_for_level(0, 100)) {
        return false;
    }

    // Lectura de los 40 bits de datos
    for (int i = 0; i < 40; i++) {
        if (!dht11_wait_for_level(1, 100)) {
            return false;
        }

        esp_rom_delay_us(40); // Muestreo a los 40us para determinar bit 0 o bit 1
        int level = gpio_get_level(DHT11_GPIO);

        int byte_index = i / 8;
        int bit_index = 7 - (i % 8);

        if (level == 1) {
            data[byte_index] |= (1 << bit_index);
        }

        if (!dht11_wait_for_level(0, 100)) {
            return false;
        }
    }

    // Verificación de Checksum
    uint8_t checksum = data[0] + data[1] + data[2] + data[3];
    if (checksum != data[4]) {
        ESP_LOGW(TAG, "DHT11 checksum incorrecto");
        return false;
    }

    // Asignación de datos parseados
    *humidity = data[0] + (data[1] / 10.0f);
    *temperature = data[2] + (data[3] / 10.0f);

    return true;
}

/**
 * @brief Tarea FreeRTOS para la lectura periódica del sensor DHT11.
 * @param pvParameters Parámetros de la tarea (no utilizado).
 */
static void dht11_task(void *pvParameters) {
    float humidity;
    float temperature;

    while (1) {
        if (dht11_read(&humidity, &temperature)) {
            ESP_LOGI(TAG, "DHT11 -> Temperatura: %.1f C | Humedad: %.1f %%", temperature, humidity);
        } else {
            ESP_LOGW(TAG, "DHT11 -> Error de lectura");
        }

        vTaskDelay(pdMS_TO_TICKS(DHT11_INTERVAL_MS));
    }
}

// ============================================================
// PUNTO DE ENTRADA PRINCIPAL
// ============================================================

/**
 * @brief Función de entrada principal de la aplicación.
 * @details Inicializa los periféricos, crea la cola FreeRTOS y arranca las 4 tareas
 *          concurrentes (relés, puerto serie, LM35 y DHT11).
 */
void app_main(void) {
    ESP_LOGI(TAG, "==========================================");
    ESP_LOGI(TAG, " ESP32-S3 - SISTEMA DE CONTROL");
    ESP_LOGI(TAG, " ESP-IDF + FreeRTOS");
    ESP_LOGI(TAG, "==========================================");

    // Inicialización de hardware
    relay_gpio_init();
    lm35_adc1_init();
    lm35_adc2_init();
    dht11_init();

    // Creación de colas
    relay_queue = xQueueCreate(10, sizeof(relay_command_t));
    if (relay_queue == NULL) {
        ESP_LOGE(TAG, "ERROR creando cola");
        return;
    }

    // Creación de tareas
    if (xTaskCreate(relay_task, "relay_task", 4096, NULL, 5, NULL) != pdPASS) {
        ESP_LOGE(TAG, "ERROR creando relay_task");
        return;
    }

    if (xTaskCreate(serial_task, "serial_task", 4096, NULL, 4, NULL) != pdPASS) {
        ESP_LOGE(TAG, "ERROR creando serial_task");
        return;
    }

    if (xTaskCreate(lm35_task, "lm35_task", 4096, NULL, 3, NULL) != pdPASS) {
        ESP_LOGE(TAG, "ERROR creando lm35_task");
        return;
    }

    if (xTaskCreate(dht11_task, "dht11_task", 4096, NULL, 3, NULL) != pdPASS) {
        ESP_LOGE(TAG, "ERROR creando dht11_task");
        return;
    }

    // Resumen del sistema
    ESP_LOGI(TAG, "Sistema iniciado correctamente");
    ESP_LOGI(TAG, "RELES: GPIO 35, 36, 37, 38, 39");
    ESP_LOGI(TAG, "LM35 #1: GPIO 9");
    ESP_LOGI(TAG, "LM35 #2: GPIO 11");
    ESP_LOGI(TAG, "DHT11: GPIO 10");
    ESP_LOGI(TAG, "Comandos: :1 hasta :5");
}