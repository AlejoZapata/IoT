#include <stdint.h>
#include "esp_err.h"

typedef struct {
    int trig_pin;
    int echo_pin;
} hcsr04_config_t;

esp_err_t hcsr04_init(const hcsr04_config_t *cfg);
esp_err_t hcsr04_measure_cm(const hcsr04_config_t *cfg, float *out_cm);