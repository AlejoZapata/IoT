#include "ssd1306.h"
#include "ssd1306_font.h"
#include <string.h>
#include <stdlib.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define TAG "SSD1306"

static i2c_port_t s_i2c_port = I2C_NUM_0;
static uint8_t s_i2c_addr = SSD1306_DEFAULT_ADDR;
static uint8_t s_buffer[SSD1306_BUFFER_SIZE];

static esp_err_t ssd1306_write_cmd(uint8_t cmd) {
    uint8_t buf[2] = {0x00, cmd};
    return i2c_master_write_to_device(s_i2c_port, s_i2c_addr, buf, sizeof(buf), pdMS_TO_TICKS(100));
}

static esp_err_t ssd1306_write_cmds(const uint8_t *cmds, size_t len) {
    i2c_cmd_handle_t handle = i2c_cmd_link_create();
    i2c_master_start(handle);
    i2c_master_write_byte(handle, (s_i2c_addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(handle, 0x00, true); // Control: Command stream
    i2c_master_write(handle, (uint8_t *)cmds, len, true);
    i2c_master_stop(handle);
    esp_err_t ret = i2c_master_cmd_begin(s_i2c_port, handle, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(handle);
    return ret;
}

esp_err_t ssd1306_init(i2c_port_t port, uint8_t addr) {
    s_i2c_port = port;
    s_i2c_addr = addr;

    // Secuencia de inicialización estándar para SSD1306 128x64
    const uint8_t init_cmds[] = {
        0xAE,       // Display OFF
        0xD5, 0x80, // Set Display Clock Divide Ratio / Oscillator Frequency
        0xA8, 0x3F, // Set Multiplex Ratio (64 - 1 = 63 = 0x3F)
        0xD3, 0x00, // Set Display Offset (0)
        0x40,       // Set Display Start Line (0)
        0x8D, 0x14, // Enable Charge Pump (0x14: 7.5V)
        0x20, 0x00, // Set Memory Addressing Mode (0x00: Horizontal)
        0xA1,       // Set Segment Re-map (0xA1: column 127 is SEG0)
        0xC8,       // Set COM Output Scan Direction (0xC8: remapped mode)
        0xDA, 0x12, // Set COM Pins Hardware Configuration (0x12: alternative)
        0x81, 0xCF, // Set Contrast Control (0xCF)
        0xD9, 0xF1, // Set Pre-charge Period (Phase 1: 1 DCLK, Phase 2: 15 DCLK)
        0xDB, 0x40, // Set VCOMH Deselect Level (0x40: ~0.77 x Vcc)
        0xA4,       // Entire Display ON (output follows RAM content)
        0xA6,       // Set Normal Display (0xA6: normal, 0xA7: inverse)
        0x2E,       // Deactivate Scroll
        0xAF        // Display ON
    };

    esp_err_t ret = ssd1306_write_cmds(init_cmds, sizeof(init_cmds));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error inicializando SSD1306 (addr: 0x%02X): %s", addr, esp_err_to_name(ret));
        return ret;
    }

    ssd1306_clear();
    ret = ssd1306_update();
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "SSD1306 128x64 inicializado exitosamente en 0x%02X", addr);
    }
    return ret;
}

void ssd1306_clear(void) {
    memset(s_buffer, 0, sizeof(s_buffer));
}

void ssd1306_fill(uint8_t color) {
    memset(s_buffer, color ? 0xFF : 0x00, sizeof(s_buffer));
}

esp_err_t ssd1306_update(void) {
    // Configurar rangos de columnas (0-127) y páginas (0-7)
    const uint8_t page_cmds[] = {
        0x21, 0x00, 0x7F, // Column address: 0 a 127
        0x22, 0x00, 0x07  // Page address: 0 a 7
    };
    esp_err_t ret = ssd1306_write_cmds(page_cmds, sizeof(page_cmds));
    if (ret != ESP_OK) {
        return ret;
    }

    // Transmitir buffer gráfico
    i2c_cmd_handle_t handle = i2c_cmd_link_create();
    i2c_master_start(handle);
    i2c_master_write_byte(handle, (s_i2c_addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(handle, 0x40, true); // Control byte: Data stream
    i2c_master_write(handle, s_buffer, sizeof(s_buffer), true);
    i2c_master_stop(handle);

    ret = i2c_master_cmd_begin(s_i2c_port, handle, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(handle);
    return ret;
}

void ssd1306_draw_pixel(int16_t x, int16_t y, uint8_t color) {
    if (x < 0 || x >= SSD1306_WIDTH || y < 0 || y >= SSD1306_HEIGHT) {
        return;
    }

    uint16_t idx = x + (y / 8) * SSD1306_WIDTH;
    uint8_t bit_mask = (1 << (y % 8));

    if (color == SSD1306_COLOR_WHITE) {
        s_buffer[idx] |= bit_mask;
    } else if (color == SSD1306_COLOR_BLACK) {
        s_buffer[idx] &= ~bit_mask;
    } else if (color == SSD1306_COLOR_INVERT) {
        s_buffer[idx] ^= bit_mask;
    }
}

void ssd1306_draw_fast_h_line(int16_t x, int16_t y, int16_t w, uint8_t color) {
    if (y < 0 || y >= SSD1306_HEIGHT || w <= 0) return;
    if (x < 0) {
        w += x;
        x = 0;
    }
    if (x + w > SSD1306_WIDTH) {
        w = SSD1306_WIDTH - x;
    }
    if (w <= 0) return;

    for (int16_t i = 0; i < w; i++) {
        ssd1306_draw_pixel(x + i, y, color);
    }
}

void ssd1306_draw_fast_v_line(int16_t x, int16_t y, int16_t h, uint8_t color) {
    if (x < 0 || x >= SSD1306_WIDTH || h <= 0) return;
    if (y < 0) {
        h += y;
        y = 0;
    }
    if (y + h > SSD1306_HEIGHT) {
        h = SSD1306_HEIGHT - y;
    }
    if (h <= 0) return;

    for (int16_t i = 0; i < h; i++) {
        ssd1306_draw_pixel(x, y + i, color);
    }
}

void ssd1306_draw_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t color) {
    int16_t dx = abs(x1 - x0);
    int16_t sx = x0 < x1 ? 1 : -1;
    int16_t dy = -abs(y1 - y0);
    int16_t sy = y0 < y1 ? 1 : -1;
    int16_t err = dx + dy;

    while (1) {
        ssd1306_draw_pixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        int16_t e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

void ssd1306_draw_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color) {
    if (w <= 0 || h <= 0) return;
    ssd1306_draw_fast_h_line(x, y, w, color);
    ssd1306_draw_fast_h_line(x, y + h - 1, w, color);
    ssd1306_draw_fast_v_line(x, y, h, color);
    ssd1306_draw_fast_v_line(x + w - 1, y, h, color);
}

void ssd1306_fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color) {
    if (w <= 0 || h <= 0) return;
    for (int16_t i = y; i < y + h; i++) {
        ssd1306_draw_fast_h_line(x, i, w, color);
    }
}

void ssd1306_draw_circle(int16_t x0, int16_t y0, int16_t r, uint8_t color) {
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;

    ssd1306_draw_pixel(x0, y0 + r, color);
    ssd1306_draw_pixel(x0, y0 - r, color);
    ssd1306_draw_pixel(x0 + r, y0, color);
    ssd1306_draw_pixel(x0 - r, y0, color);

    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;

        ssd1306_draw_pixel(x0 + x, y0 + y, color);
        ssd1306_draw_pixel(x0 - x, y0 + y, color);
        ssd1306_draw_pixel(x0 + x, y0 - y, color);
        ssd1306_draw_pixel(x0 - x, y0 - y, color);
        ssd1306_draw_pixel(x0 + y, y0 + x, color);
        ssd1306_draw_pixel(x0 - y, y0 + x, color);
        ssd1306_draw_pixel(x0 + y, y0 - x, color);
        ssd1306_draw_pixel(x0 - y, y0 - x, color);
    }
}

void ssd1306_fill_circle(int16_t x0, int16_t y0, int16_t r, uint8_t color) {
    ssd1306_draw_fast_v_line(x0, y0 - r, 2 * r + 1, color);

    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;

    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;

        ssd1306_draw_fast_v_line(x0 + x, y0 - y, 2 * y + 1, color);
        ssd1306_draw_fast_v_line(x0 - x, y0 - y, 2 * y + 1, color);
        ssd1306_draw_fast_v_line(x0 + y, y0 - x, 2 * x + 1, color);
        ssd1306_draw_fast_v_line(x0 - y, y0 - x, 2 * x + 1, color);
    }
}

void ssd1306_draw_char(int16_t x, int16_t y, char c, uint8_t color, uint8_t bg_color) {
    if (c < 32 || c > 126) {
        c = '?';
    }

    uint8_t char_index = c - 32;

    for (int col = 0; col < 5; col++) {
        uint8_t line = font5x7[char_index][col];
        for (int row = 0; row < 8; row++) {
            if (line & (1 << row)) {
                ssd1306_draw_pixel(x + col, y + row, color);
            } else if (bg_color != color) {
                ssd1306_draw_pixel(x + col, y + row, bg_color);
            }
        }
    }
    // Columna de espacio entre caracteres (ancho total = 6 pixels)
    if (bg_color != color) {
        for (int row = 0; row < 8; row++) {
            ssd1306_draw_pixel(x + 5, y + row, bg_color);
        }
    }
}

void ssd1306_draw_string(int16_t x, int16_t y, const char *str, uint8_t color, uint8_t bg_color) {
    int16_t cursor_x = x;
    int16_t cursor_y = y;

    while (*str) {
        if (*str == '\n') {
            cursor_y += 9;
            cursor_x = x;
        } else if (*str == '\r') {
            // ignorar retorno de carro
        } else {
            if (cursor_x + 6 > SSD1306_WIDTH) {
                cursor_y += 9;
                cursor_x = x;
            }
            ssd1306_draw_char(cursor_x, cursor_y, *str, color, bg_color);
            cursor_x += 6;
        }
        str++;
    }
}

void ssd1306_draw_bitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, uint8_t color) {
    int16_t byte_width = (w + 7) / 8;
    for (int16_t j = 0; j < h; j++) {
        for (int16_t i = 0; i < w; i++) {
            if (bitmap[j * byte_width + (i / 8)] & (128 >> (i & 7))) {
                ssd1306_draw_pixel(x + i, y + j, color);
            }
        }
    }
}

esp_err_t ssd1306_set_contrast(uint8_t contrast) {
    uint8_t cmds[] = {0x81, contrast};
    return ssd1306_write_cmds(cmds, sizeof(cmds));
}

esp_err_t ssd1306_invert_display(bool invert) {
    return ssd1306_write_cmd(invert ? 0xA7 : 0xA6);
}

uint8_t* ssd1306_get_buffer(void) {
    return s_buffer;
}
