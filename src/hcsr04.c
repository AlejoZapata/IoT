#include "hcsr04.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#define TAG "HCSR04"
#define TIMEOUT_US 25000   // 25ms → ~4 metros máximo

esp_err_t hcsr04_init(const hcsr04_config_t *cfg) {
    // Configurar TRIG como salida
    gpio_config_t trig_conf = {
        .pin_bit_mask = (1ULL << cfg->trig_pin),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&trig_conf));

    // Configurar ECHO como entrada
    gpio_config_t echo_conf = {
        .pin_bit_mask = (1ULL << cfg->echo_pin),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&echo_conf));

    gpio_set_level(cfg->trig_pin, 0);
    return ESP_OK;
}

esp_err_t hcsr04_measure_cm(const hcsr04_config_t *cfg, float *out_cm) {
    // Pulso TRIG de 10µs
    gpio_set_level(cfg->trig_pin, 1);
    esp_rom_delay_us(10);
    gpio_set_level(cfg->trig_pin, 0);

    // Esperar flanco de subida en ECHO
    int64_t t_start = esp_timer_get_time();
    while (gpio_get_level(cfg->echo_pin) == 0) {
        if ((esp_timer_get_time() - t_start) > TIMEOUT_US) {
            ESP_LOGW(TAG, "Timeout esperando flanco de subida");
            return ESP_ERR_TIMEOUT;
        }
    }

    // Medir duración del pulso ECHO
    int64_t t_high = esp_timer_get_time();
    while (gpio_get_level(cfg->echo_pin) == 1) {
        if ((esp_timer_get_time() - t_high) > TIMEOUT_US) {
            ESP_LOGW(TAG, "Timeout esperando flanco de bajada");
            return ESP_ERR_TIMEOUT;
        }
    }
    int64_t t_end = esp_timer_get_time();

    // Velocidad del sonido: 340 m/s → 0.034 cm/µs, ida y vuelta ÷ 2
    *out_cm = (float)(t_end - t_high) * 0.034f / 2.0f;
    return ESP_OK;
}