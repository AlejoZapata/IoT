#include "dht11.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "rom/ets_sys.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define TAG "DHT11"

// Esperar un nivel en el pin con timeout en µs
static esp_err_t wait_level(int pin, int level, int timeout_us)
{
    int64_t start = esp_timer_get_time();
    while (gpio_get_level(pin) != level) {
        if ((esp_timer_get_time() - start) > timeout_us)
            return ESP_ERR_TIMEOUT;
    }
    return ESP_OK;
}

esp_err_t dht11_init(int gpio_pin)
{
    gpio_reset_pin(gpio_pin);
    gpio_set_direction(gpio_pin, GPIO_MODE_INPUT_OUTPUT_OD);
    gpio_set_pull_mode(gpio_pin, GPIO_PULLUP_ONLY);
    gpio_set_direction(gpio_pin, GPIO_MODE_OUTPUT);
    gpio_set_level(gpio_pin, 1);
    return ESP_OK;
}

esp_err_t dht11_read(int gpio_pin, dht11_data_t *out)
{
    uint8_t data[5] = {0};

    // Señal de inicio del maestro: mínimo 18ms LOW
    gpio_set_direction(gpio_pin, GPIO_MODE_OUTPUT);
    gpio_set_level(gpio_pin, 0);
    ets_delay_us(20000); // 20ms en nivel bajo
    gpio_set_level(gpio_pin, 1);
    ets_delay_us(30);

    // Cambiar a entrada con pull-up para recibir respuesta
    gpio_set_direction(gpio_pin, GPIO_MODE_INPUT);
    gpio_set_pull_mode(gpio_pin, GPIO_PULLUP_ONLY);

    // Sección crítica para evitar que interrupciones o cambios de tarea
    // alteren los tiempos de medición en microsegundos
    portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
    taskENTER_CRITICAL(&mux);

    if (wait_level(gpio_pin, 0, 100) != ESP_OK) {
        taskEXIT_CRITICAL(&mux);
        ESP_LOGD(TAG, "Sin respuesta (flanco bajo)");
        return ESP_ERR_TIMEOUT;
    }
    if (wait_level(gpio_pin, 1, 100) != ESP_OK) {
        taskEXIT_CRITICAL(&mux);
        ESP_LOGD(TAG, "Sin respuesta (flanco alto)");
        return ESP_ERR_TIMEOUT;
    }
    if (wait_level(gpio_pin, 0, 100) != ESP_OK) {
        taskEXIT_CRITICAL(&mux);
        ESP_LOGD(TAG, "Sin respuesta (inicio datos)");
        return ESP_ERR_TIMEOUT;
    }

    // Leer 40 bits (5 bytes)
    for (int i = 0; i < 40; i++)
    {
        // Esperar flanco de subida (inicio de bit)
        if (wait_level(gpio_pin, 1, 100) != ESP_OK) {
            taskEXIT_CRITICAL(&mux);
            return ESP_ERR_TIMEOUT;
        }

        // Medir duración del nivel alto
        // < 30µs → bit 0 | > 30µs → bit 1
        ets_delay_us(35);
        if (gpio_get_level(gpio_pin) == 1) {
            data[i / 8] |= (1 << (7 - (i % 8)));
            // Esperar que termine el bit 1
            if (wait_level(gpio_pin, 0, 100) != ESP_OK) {
                taskEXIT_CRITICAL(&mux);
                return ESP_ERR_TIMEOUT;
            }
        }
    }

    taskEXIT_CRITICAL(&mux);

    // Verificar checksum
    uint8_t checksum = data[0] + data[1] + data[2] + data[3];
    if (checksum != data[4]) {
        ESP_LOGD(TAG, "Checksum inválido: %02X != %02X", checksum, data[4]);
        return ESP_ERR_INVALID_CRC;
    }

    out->humidity    = (float)data[0] + (float)data[1] * 0.1f;
    out->temperature = (float)data[2] + (float)data[3] * 0.1f;

    return ESP_OK;
}
