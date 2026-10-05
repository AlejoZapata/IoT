#ifndef SSD1306_H
#define SSD1306_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "driver/i2c.h"

#define SSD1306_WIDTH       128
#define SSD1306_HEIGHT      64
#define SSD1306_BUFFER_SIZE (SSD1306_WIDTH * SSD1306_HEIGHT / 8)

// Dirección I2C típica del SSD1306 (0x3C o a veces 0x3D)
#define SSD1306_DEFAULT_ADDR 0x3C

// Colores lógicos
#define SSD1306_COLOR_BLACK  0
#define SSD1306_COLOR_WHITE  1
#define SSD1306_COLOR_INVERT 2

/**
 * @brief Inicializa el display OLED SSD1306 en el bus I2C especificado.
 * 
 * @param port Puerto I2C ya inicializado (por ej. I2C_NUM_0)
 * @param addr Dirección I2C del display (típicamente 0x3C)
 * @return ESP_OK si la inicialización fue exitosa, o código de error
 */
esp_err_t ssd1306_init(i2c_port_t port, uint8_t addr);

/**
 * @brief Limpia el buffer interno (todo apagado). No actualiza la pantalla de inmediato.
 */
void ssd1306_clear(void);

/**
 * @brief Llena el buffer interno (todo encendido o todo apagado).
 */
void ssd1306_fill(uint8_t color);

/**
 * @brief Envía el buffer gráfico completo al display mediante I2C.
 * 
 * @return ESP_OK si la transmisión fue correcta
 */
esp_err_t ssd1306_update(void);

/**
 * @brief Dibuja un pixel en las coordenadas dadas.
 */
void ssd1306_draw_pixel(int16_t x, int16_t y, uint8_t color);

/**
 * @brief Dibuja una línea entre dos puntos usando el algoritmo de Bresenham.
 */
void ssd1306_draw_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t color);

/**
 * @brief Dibuja una línea horizontal optimizada.
 */
void ssd1306_draw_fast_h_line(int16_t x, int16_t y, int16_t w, uint8_t color);

/**
 * @brief Dibuja una línea vertical optimizada.
 */
void ssd1306_draw_fast_v_line(int16_t x, int16_t y, int16_t h, uint8_t color);

/**
 * @brief Dibuja el contorno de un rectángulo.
 */
void ssd1306_draw_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color);

/**
 * @brief Dibuja un rectángulo relleno.
 */
void ssd1306_fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color);

/**
 * @brief Dibuja el contorno de un círculo.
 */
void ssd1306_draw_circle(int16_t x0, int16_t y0, int16_t r, uint8_t color);

/**
 * @brief Dibuja un círculo relleno.
 */
void ssd1306_fill_circle(int16_t x0, int16_t y0, int16_t r, uint8_t color);

/**
 * @brief Dibuja un carácter en la posición (x, y). Fuente estándar 6x8.
 */
void ssd1306_draw_char(int16_t x, int16_t y, char c, uint8_t color, uint8_t bg_color);

/**
 * @brief Dibuja una cadena de texto en la posición (x, y). Salto de línea automático.
 */
void ssd1306_draw_string(int16_t x, int16_t y, const char *str, uint8_t color, uint8_t bg_color);

/**
 * @brief Dibuja un bitmap monocromo en el buffer.
 */
void ssd1306_draw_bitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, uint8_t color);

/**
 * @brief Configura el contraste del display (0 a 255).
 */
esp_err_t ssd1306_set_contrast(uint8_t contrast);

/**
 * @brief Invierte la visualización (negativo / positivo).
 */
esp_err_t ssd1306_invert_display(bool invert);

/**
 * @brief Obtiene el puntero directo al buffer de 1024 bytes.
 */
uint8_t* ssd1306_get_buffer(void);

#endif // SSD1306_H
