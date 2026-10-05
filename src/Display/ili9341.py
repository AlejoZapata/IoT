# ili9341.py - Controlador de alto rendimiento para pantalla TFT ILI9341 en MicroPython
# Optimizado para Raspberry Pi Pico / RP2040 con soporte de primitivas gráficas, fuentes y sprites

import time
import struct
from machine import Pin, SPI

# Constantes de Color RGB565 comunes
COLOR_BLACK       = 0x0000
COLOR_WHITE       = 0xFFFF
COLOR_NAVY_DARK   = 0x0842  # Fondo oscuro elegante
COLOR_CARD_BG     = 0x18E3  # Fondo de tarjeta
COLOR_CARD_BORDER = 0x3186  # Borde sutil de tarjeta
COLOR_HEADER_BG   = 0x1167  # Encabezado
COLOR_CYAN        = 0x07FF
COLOR_BLUE        = 0x243F
COLOR_GREEN       = 0x07E0
COLOR_AMBER       = 0xFD20
COLOR_RED         = 0xF800
COLOR_PURPLE      = 0x981F
COLOR_GRAY_DARK   = 0x39E7
COLOR_GRAY_LIGHT  = 0xBDD7

# Fuente matricial 5x7 compacta (ASCII 32 a 126 + simbolo de grado en 127)
# Cada caracter ocupa 5 columnas de 7 bits
FONT_5x7 = {
    ' ': (0x00, 0x00, 0x00, 0x00, 0x00),
    '!': (0x00, 0x00, 0x5F, 0x00, 0x00),
    '"': (0x00, 0x07, 0x00, 0x07, 0x00),
    '#': (0x14, 0x7F, 0x14, 0x7F, 0x14),
    '$': (0x24, 0x2A, 0x7F, 0x2A, 0x12),
    '%': (0x23, 0x13, 0x08, 0x64, 0x62),
    '&': (0x36, 0x49, 0x55, 0x22, 0x50),
    "'": (0x00, 0x05, 0x03, 0x00, 0x00),
    '(': (0x00, 0x1C, 0x22, 0x41, 0x00),
    ')': (0x00, 0x41, 0x22, 0x1C, 0x00),
    '*': (0x14, 0x08, 0x3E, 0x08, 0x14),
    '+': (0x08, 0x08, 0x3E, 0x08, 0x08),
    ',': (0x00, 0x50, 0x30, 0x00, 0x00),
    '-': (0x08, 0x08, 0x08, 0x08, 0x08),
    '.': (0x00, 0x60, 0x60, 0x00, 0x00),
    '/': (0x20, 0x10, 0x08, 0x04, 0x02),
    '0': (0x3E, 0x51, 0x49, 0x45, 0x3E),
    '1': (0x00, 0x42, 0x7F, 0x40, 0x00),
    '2': (0x42, 0x61, 0x51, 0x49, 0x46),
    '3': (0x21, 0x41, 0x45, 0x4B, 0x31),
    '4': (0x18, 0x14, 0x12, 0x7F, 0x10),
    '5': (0x27, 0x45, 0x45, 0x45, 0x39),
    '6': (0x3C, 0x4A, 0x49, 0x49, 0x30),
    '7': (0x01, 0x71, 0x09, 0x05, 0x03),
    '8': (0x36, 0x49, 0x49, 0x49, 0x36),
    '9': (0x06, 0x49, 0x49, 0x29, 0x1E),
    ':': (0x00, 0x36, 0x36, 0x00, 0x00),
    ';': (0x00, 0x56, 0x36, 0x00, 0x00),
    '<': (0x08, 0x14, 0x22, 0x41, 0x00),
    '=': (0x14, 0x14, 0x14, 0x14, 0x14),
    '>': (0x00, 0x41, 0x22, 0x14, 0x08),
    '?': (0x02, 0x01, 0x51, 0x09, 0x06),
    '@': (0x32, 0x49, 0x79, 0x41, 0x3E),
    'A': (0x7E, 0x11, 0x11, 0x11, 0x7E),
    'B': (0x7F, 0x49, 0x49, 0x49, 0x36),
    'C': (0x3E, 0x41, 0x41, 0x41, 0x22),
    'D': (0x7F, 0x41, 0x41, 0x22, 0x1C),
    'E': (0x7F, 0x49, 0x49, 0x49, 0x41),
    'F': (0x7F, 0x09, 0x09, 0x09, 0x01),
    'G': (0x3E, 0x41, 0x49, 0x49, 0x7A),
    'H': (0x7F, 0x08, 0x08, 0x08, 0x7F),
    'I': (0x00, 0x41, 0x7F, 0x41, 0x00),
    'J': (0x20, 0x40, 0x41, 0x3F, 0x01),
    'K': (0x7F, 0x08, 0x14, 0x22, 0x41),
    'L': (0x7F, 0x40, 0x40, 0x40, 0x40),
    'M': (0x7F, 0x02, 0x0C, 0x02, 0x7F),
    'N': (0x7F, 0x04, 0x08, 0x10, 0x7F),
    'O': (0x3E, 0x41, 0x41, 0x41, 0x3E),
    'P': (0x7F, 0x09, 0x09, 0x09, 0x06),
    'Q': (0x3E, 0x41, 0x51, 0x21, 0x5E),
    'R': (0x7F, 0x09, 0x19, 0x29, 0x46),
    'S': (0x46, 0x49, 0x49, 0x49, 0x31),
    'T': (0x01, 0x01, 0x7F, 0x01, 0x01),
    'U': (0x3F, 0x40, 0x40, 0x40, 0x3F),
    'V': (0x1F, 0x20, 0x40, 0x20, 0x1F),
    'W': (0x3F, 0x40, 0x38, 0x40, 0x3F),
    'X': (0x63, 0x14, 0x08, 0x14, 0x63),
    'Y': (0x07, 0x08, 0x70, 0x08, 0x07),
    'Z': (0x61, 0x51, 0x49, 0x45, 0x43),
    '[': (0x00, 0x7F, 0x41, 0x41, 0x00),
    '\\': (0x02, 0x04, 0x08, 0x10, 0x20),
    ']': (0x00, 0x41, 0x41, 0x7F, 0x00),
    '^': (0x04, 0x02, 0x01, 0x02, 0x04),
    '_': (0x40, 0x40, 0x40, 0x40, 0x40),
    '`': (0x00, 0x01, 0x02, 0x04, 0x00),
    'a': (0x20, 0x54, 0x54, 0x54, 0x78),
    'b': (0x7F, 0x48, 0x44, 0x44, 0x38),
    'c': (0x38, 0x44, 0x44, 0x44, 0x20),
    'd': (0x38, 0x44, 0x44, 0x48, 0x7F),
    'e': (0x38, 0x54, 0x54, 0x54, 0x18),
    'f': (0x08, 0x7E, 0x09, 0x01, 0x02),
    'g': (0x0C, 0x52, 0x52, 0x52, 0x3E),
    'h': (0x7F, 0x08, 0x04, 0x04, 0x78),
    'i': (0x00, 0x44, 0x7D, 0x40, 0x00),
    'j': (0x20, 0x40, 0x44, 0x3D, 0x00),
    'k': (0x7F, 0x10, 0x28, 0x44, 0x00),
    'l': (0x00, 0x41, 0x7F, 0x40, 0x00),
    'm': (0x7C, 0x04, 0x18, 0x04, 0x78),
    'n': (0x7C, 0x08, 0x04, 0x04, 0x78),
    'o': (0x38, 0x44, 0x44, 0x44, 0x38),
    'p': (0x7C, 0x14, 0x14, 0x14, 0x08),
    'q': (0x08, 0x14, 0x14, 0x18, 0x7C),
    'r': (0x7C, 0x08, 0x04, 0x04, 0x08),
    's': (0x48, 0x54, 0x54, 0x54, 0x20),
    't': (0x04, 0x3F, 0x44, 0x40, 0x20),
    'u': (0x3C, 0x40, 0x40, 0x20, 0x7C),
    'v': (0x1C, 0x20, 0x40, 0x20, 0x1C),
    'w': (0x3C, 0x40, 0x30, 0x40, 0x3C),
    'x': (0x44, 0x28, 0x10, 0x28, 0x44),
    'y': (0x0C, 0x50, 0x50, 0x50, 0x3C),
    'z': (0x44, 0x64, 0x54, 0x4C, 0x44),
    '{': (0x00, 0x08, 0x36, 0x41, 0x00),
    '|': (0x00, 0x00, 0x7F, 0x00, 0x00),
    '}': (0x00, 0x41, 0x36, 0x08, 0x00),
    '~': (0x08, 0x08, 0x2A, 0x1C, 0x08),
    '°': (0x00, 0x06, 0x09, 0x09, 0x06), # Grado centigrado
}

class ILI9341:
    """Controlador SPI optimizado para pantalla TFT ILI9341 de 320x240."""
    def __init__(self, spi, cs_pin, dc_pin, rst_pin=None, bl_pin=None, width=320, height=240, rotation=3):
        self.spi = spi
        self.cs = Pin(cs_pin, Pin.OUT, value=1)
        self.dc = Pin(dc_pin, Pin.OUT, value=1)
        self.rst = Pin(rst_pin, Pin.OUT, value=1) if rst_pin is not None else None
        self.bl = Pin(bl_pin, Pin.OUT, value=1) if bl_pin is not None else None
        
        self.width = width
        self.height = height
        self.rotation = rotation
        self.col_offset = 0
        self.row_offset = 0
        
        # Buffer de linea para transferencias rapidas
        self._buf2 = bytearray(2)
        self._buf4 = bytearray(4)
        
        self.reset()
        self.init()
        self.set_rotation(rotation)

    def write_cmd(self, cmd, data=None):
        """Envía comando manteniendo CS bajo durante la transferencia completa de datos."""
        self.cs.value(0)
        self.dc.value(0)
        self.spi.write(bytes([cmd]))
        if data is not None:
            self.dc.value(1)
            self.spi.write(data)
        self.cs.value(1)

    def reset(self):
        if self.rst:
            self.rst.value(1)
            time.sleep_ms(10)
            self.rst.value(0)
            time.sleep_ms(20)
            self.rst.value(1)
            time.sleep_ms(150)
        else:
            self.write_cmd(0x01) # SWRESET
            time.sleep_ms(150)

    def init(self):
        """Secuencia de inicialización nativa ST7789 con corrección de inversión de color."""
        self.write_cmd(0x01) # SWRESET
        time.sleep_ms(150)
        
        self.write_cmd(0x11) # SLPOUT (Sleep out)
        time.sleep_ms(150)
        
        self.write_cmd(0x3A, bytes([0x55])) # Pixel format: 16-bit RGB565
        time.sleep_ms(10)
        
        # Inversión de color: 0x20 (INVOFF) para modo nativo
        self.write_cmd(0x20) # INVOFF
        time.sleep_ms(10)
        
        self.write_cmd(0x13) # NORON (Normal display mode)
        time.sleep_ms(10)
        
        # Configurar rotación
        self.set_rotation(self.rotation)
        
        self.write_cmd(0x29) # DISPON (Display on)
        time.sleep_ms(100)

    def set_inversion(self, enabled):
        """Permite activar (0x21) o desactivar (0x20) la inversión de color en caliente."""
        if enabled:
            self.write_cmd(0x21) # INVON
        else:
            self.write_cmd(0x20) # INVOFF

    def set_rotation(self, rot, bgr=True):
        """
        Rotaciones estándar ST7789 para resolución 240x320 / 320x240:
        0: Retrato normal (240x320)
        1: Paisaje horizontal (320x240) - MADCTL 0x70 (o 0x60)
        2: Retrato invertido (240x320)  - MADCTL 0xC8 (o 0xC0)
        3: Paisaje horizontal 180°      - MADCTL 0xA8 (o 0xA0)
        """
        self.rotation = rot % 4
        bgr_bit = 0x08 if bgr else 0x00
        
        if self.rotation == 0:
            madctl = 0x00 | bgr_bit
            self.width, self.height = 240, 320
            self.col_offset, self.row_offset = 0, 0
        elif self.rotation == 1:
            madctl = 0x60 | bgr_bit # MV=1, MX=1 -> 0x70 con BGR, 0x60 con RGB
            self.width, self.height = 320, 240
            self.col_offset, self.row_offset = 0, 0
        elif self.rotation == 2:
            madctl = 0xC0 | bgr_bit # MY=1, MX=1
            self.width, self.height = 240, 320
            self.col_offset, self.row_offset = 0, 0
        else: # 3
            madctl = 0xA0 | bgr_bit # MV=1, MY=1 -> 0xA8 con BGR, 0xA0 con RGB
            self.width, self.height = 320, 240
            self.col_offset, self.row_offset = 0, 0
            
        self.write_cmd(0x36, bytes([madctl]))

    def clear_ram(self, color=0x0000):
        """Limpia los 320x320 pixeles completos del framebuffer interno para eliminar ruido residual."""
        self.set_window(0, 0, 319, 319)
        hi = (color >> 8) & 0xFF
        lo = color & 0xFF
        chunk = bytearray([hi, lo] * 256)
        total = 320 * 320
        while total > 0:
            to_send = min(total, 256)
            self.spi.write(memoryview(chunk)[:to_send * 2])
            total -= to_send
        self.cs.value(1)

    def set_window(self, x0, y0, x1, y1):
        """Define el area de memoria a escribir manteniendo CS bajo de forma continua."""
        x0 += self.col_offset
        x1 += self.col_offset
        y0 += self.row_offset
        y1 += self.row_offset
        
        self.cs.value(0)
        
        # Columna (X) 0x2A
        self.dc.value(0)
        self.spi.write(b'\x2A')
        self.dc.value(1)
        self._buf4[0] = (x0 >> 8) & 0xFF
        self._buf4[1] = x0 & 0xFF
        self._buf4[2] = (x1 >> 8) & 0xFF
        self._buf4[3] = x1 & 0xFF
        self.spi.write(self._buf4)
        
        # Fila (Y) 0x2B
        self.dc.value(0)
        self.spi.write(b'\x2B')
        self.dc.value(1)
        self._buf4[0] = (y0 >> 8) & 0xFF
        self._buf4[1] = y0 & 0xFF
        self._buf4[2] = (y1 >> 8) & 0xFF
        self._buf4[3] = y1 & 0xFF
        self.spi.write(self._buf4)
        
        # Inicio de escritura en RAM (0x2C)
        self.dc.value(0)
        self.spi.write(b'\x2C')
        self.dc.value(1)
        # Nota: CS se mantiene en 0 para que fill_rect envie los pixeles de inmediato

    def fill_rect(self, x, y, w, h, color):
        """Rellena un rectangulo con un color RGB565 mediante chunks optimizados."""
        if x >= self.width or y >= self.height or w <= 0 or h <= 0:
            return
        x1 = min(x + w - 1, self.width - 1)
        y1 = min(y + h - 1, self.height - 1)
        real_w = x1 - x + 1
        real_h = y1 - y + 1
        
        self.set_window(x, y, x1, y1)
        
        # Buffer de llenado por lotes
        hi = (color >> 8) & 0xFF
        lo = color & 0xFF
        total_pixels = real_w * real_h
        chunk_pixels = min(total_pixels, 512)
        buf = bytearray([hi, lo] * chunk_pixels)
        
        self.dc.value(1)
        self.cs.value(0)
        remaining = total_pixels
        while remaining > 0:
            to_send = min(remaining, chunk_pixels)
            if to_send == chunk_pixels:
                self.spi.write(buf)
            else:
                self.spi.write(memoryview(buf)[:to_send * 2])
            remaining -= to_send
        self.cs.value(1)

    def fill_screen(self, color):
        """Limpia la pantalla completa con un color solido."""
        self.fill_rect(0, 0, self.width, self.height, color)

    def draw_rect(self, x, y, w, h, color):
        """Dibuja el borde de un rectangulo."""
        self.fill_rect(x, y, w, 1, color)
        self.fill_rect(x, y + h - 1, w, 1, color)
        self.fill_rect(x, y, 1, h, color)
        self.fill_rect(x + w - 1, y, 1, h, color)

    def draw_hline(self, x, y, w, color):
        """Dibuja una linea horizontal."""
        self.fill_rect(x, y, w, 1, color)

    def draw_vline(self, x, y, h, color):
        """Dibuja una linea vertical."""
        self.fill_rect(x, y, 1, h, color)

    def draw_char(self, char, x, y, color, bg_color=None, scale=1):
        """Dibuja un caracter con escalamiento opcional (1x, 2x)."""
        glyph = FONT_5x7.get(char, FONT_5x7.get('?'))
        char_w = 6 * scale
        char_h = 8 * scale
        
        if bg_color is not None:
            self.fill_rect(x, y, char_w, char_h, bg_color)
            
        for col_idx in range(5):
            col_bits = glyph[col_idx]
            for row_idx in range(7):
                if (col_bits >> row_idx) & 1:
                    px = x + col_idx * scale
                    py = y + row_idx * scale
                    if scale == 1:
                        self.fill_rect(px, py, 1, 1, color)
                    else:
                        self.fill_rect(px, py, scale, scale, color)

    def draw_text(self, text, x, y, color, bg_color=None, scale=1):
        """Dibuja una cadena de texto en las coordenadas dadas."""
        cursor_x = x
        for c in text:
            if cursor_x + (6 * scale) > self.width:
                break
            self.draw_char(c, cursor_x, y, color, bg_color, scale)
            cursor_x += 6 * scale

    def draw_bitmap_16x16(self, matrix, x, y, palette, scale=1):
        """
        Dibuja una matriz de caracteres 16x16 usando una paleta de colores.
        palette es un diccionario: {'.': None, '#': color1, 'x': color2, '@': color3}
        Si el color es None, se dibuja transparente.
        """
        for r_idx, row in enumerate(matrix):
            for c_idx, char in enumerate(row):
                color = palette.get(char)
                if color is not None:
                    px = x + c_idx * scale
                    py = y + r_idx * scale
                    if scale == 1:
                        self.fill_rect(px, py, 1, 1, color)
                    else:
                        self.fill_rect(px, py, scale, scale, color)
