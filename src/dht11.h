#include <stdint.h>
#include "esp_err.h"

typedef struct {
    float temperature;
    float humidity;
} dht11_data_t;

esp_err_t dht11_init(int gpio_pin);
esp_err_t dht11_read(int gpio_pin, dht11_data_t *out);
