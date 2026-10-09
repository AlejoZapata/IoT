#ifndef SSD1306_H
#define SSD1306_H

/**
 * @file ssd1306.h
 * @brief API grafica para un display OLED SSD1306 de 128x64 conectado por I2C.
 *
 * Las operaciones de dibujo modifican un buffer en RAM. Para mostrar los
 * cambios en el display se debe llamar a ssd1306_update().
 */

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "driver/i2c.h"

/** Ancho del display en pixeles. */
#define SSD1306_WIDTH       128
/** Alto del display en pixeles. */
#define SSD1306_HEIGHT      64
/** Tamano del buffer monocromo, expresado en bytes. */
#define SSD1306_BUFFER_SIZE (SSD1306_WIDTH * SSD1306_HEIGHT / 8)

/** Direccion I2C predeterminada del modulo (algunos usan 0x3D). */
#define SSD1306_DEFAULT_ADDR 0x3C

/** Pixel apagado. */
#define SSD1306_COLOR_BLACK  0
/** Pixel encendido. */
#define SSD1306_COLOR_WHITE  1
/** Invierte el estado actual del pixel. */
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
 * @param x Coordenada horizontal, desde 0.
 * @param y Coordenada vertical, desde 0.
 * @param color SSD1306_COLOR_BLACK, SSD1306_COLOR_WHITE o SSD1306_COLOR_INVERT.
 * @note Las coordenadas fuera del display se ignoran.
 */
void ssd1306_draw_pixel(int16_t x, int16_t y, uint8_t color);

/**
 * @brief Dibuja una linea entre dos puntos usando el algoritmo de Bresenham.
 * @param x0 Coordenada horizontal inicial.
 * @param y0 Coordenada vertical inicial.
 * @param x1 Coordenada horizontal final.
 * @param y1 Coordenada vertical final.
 * @param color Operacion de color aplicada a los pixeles.
 */
void ssd1306_draw_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t color);

/**
 * @brief Dibuja una linea horizontal.
 * @param x Coordenada horizontal inicial.
 * @param y Coordenada vertical.
 * @param w Longitud en pixeles; debe ser positiva.
 * @param color Operacion de color aplicada a los pixeles.
 */
void ssd1306_draw_fast_h_line(int16_t x, int16_t y, int16_t w, uint8_t color);

/**
 * @brief Dibuja una linea vertical.
 * @param x Coordenada horizontal.
 * @param y Coordenada vertical inicial.
 * @param h Longitud en pixeles; debe ser positiva.
 * @param color Operacion de color aplicada a los pixeles.
 */
void ssd1306_draw_fast_v_line(int16_t x, int16_t y, int16_t h, uint8_t color);

/**
 * @brief Dibuja el contorno de un rectangulo.
 * @param x Coordenada horizontal de la esquina superior izquierda.
 * @param y Coordenada vertical de la esquina superior izquierda.
 * @param w Ancho del rectangulo en pixeles.
 * @param h Alto del rectangulo en pixeles.
 * @param color Operacion de color aplicada a los pixeles.
 */
void ssd1306_draw_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color);

/**
 * @brief Dibuja un rectangulo relleno.
 * @param x Coordenada horizontal de la esquina superior izquierda.
 * @param y Coordenada vertical de la esquina superior izquierda.
 * @param w Ancho del rectangulo en pixeles.
 * @param h Alto del rectangulo en pixeles.
 * @param color Operacion de color aplicada a los pixeles.
 */
void ssd1306_fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color);

/**
 * @brief Dibuja el contorno de un circulo.
 * @param x0 Coordenada horizontal del centro.
 * @param y0 Coordenada vertical del centro.
 * @param r Radio en pixeles.
 * @param color Operacion de color aplicada a los pixeles.
 */
void ssd1306_draw_circle(int16_t x0, int16_t y0, int16_t r, uint8_t color);

/**
 * @brief Dibuja un circulo relleno.
 * @param x0 Coordenada horizontal del centro.
 * @param y0 Coordenada vertical del centro.
 * @param r Radio en pixeles.
 * @param color Operacion de color aplicada a los pixeles.
 */
void ssd1306_fill_circle(int16_t x0, int16_t y0, int16_t r, uint8_t color);

/**
 * @brief Dibuja un caracter en la posicion indicada usando una celda de 6x8.
 * @param x Coordenada horizontal de la esquina superior izquierda.
 * @param y Coordenada vertical de la esquina superior izquierda.
 * @param c Caracter ASCII imprimible; los valores fuera del rango se sustituyen por '?'.
 * @param color Color logico de los pixeles activos.
 * @param bg_color Color de fondo; si coincide con @p color, el fondo no se dibuja.
 */
void ssd1306_draw_char(int16_t x, int16_t y, char c, uint8_t color, uint8_t bg_color);

/**
 * @brief Dibuja una cadena de texto con salto de linea automatico.
 * @param x Coordenada horizontal inicial.
 * @param y Coordenada vertical inicial.
 * @param str Cadena terminada en nulo; no puede ser NULL.
 * @param color Color logico de los pixeles activos.
 * @param bg_color Color de fondo; si coincide con @p color, el fondo no se dibuja.
 * @note El salto de linea explicito y el ajuste al borde avanzan 9 pixeles.
 */
void ssd1306_draw_string(int16_t x, int16_t y, const char *str, uint8_t color, uint8_t bg_color);

/**
 * @brief Dibuja los pixeles activos de un bitmap monocromo en el buffer.
 * @param x Coordenada horizontal de destino.
 * @param y Coordenada vertical de destino.
 * @param bitmap Datos del bitmap, organizados por filas con el bit mas significativo primero.
 * @param w Ancho del bitmap en pixeles.
 * @param h Alto del bitmap en pixeles.
 * @param color Color aplicado a los bits activos; los bits inactivos no modifican el buffer.
 */
void ssd1306_draw_bitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, uint8_t color);

/**
 * @brief Configura el contraste del display.
 * @param contrast Valor de contraste entre 0 y 255.
 * @return ESP_OK si el comando se envia correctamente; en otro caso, el error I2C.
 */
esp_err_t ssd1306_set_contrast(uint8_t contrast);

/**
 * @brief Activa o desactiva la inversion de la imagen en el display.
 * @param invert true para invertir la imagen; false para el modo normal.
 * @return ESP_OK si el comando se envia correctamente; en otro caso, el error I2C.
 */
esp_err_t ssd1306_invert_display(bool invert);

/**
 * @brief Obtiene el puntero modificable al buffer grafico interno.
 * @return Puntero al buffer de SSD1306_BUFFER_SIZE bytes, almacenado por paginas.
 * @note Los cambios directos solo se envian al display al llamar a ssd1306_update().
 */
uint8_t* ssd1306_get_buffer(void);

#endif // SSD1306_H
