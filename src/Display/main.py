# main.py - Sistema de Monitoreo en Pantalla TFT ILI9341 para Raspberry Pi Pico (RP2040)
# Recepcion de variables por UART y visualizacion grafica en tiempo real

import machine
import time
import sys
import select
from ili9341 import (
    ILI9341,
    COLOR_BLACK, COLOR_WHITE, COLOR_NAVY_DARK, COLOR_CARD_BG,
    COLOR_CARD_BORDER, COLOR_HEADER_BG, COLOR_CYAN, COLOR_BLUE,
    COLOR_GREEN, COLOR_AMBER, COLOR_RED, COLOR_PURPLE,
    COLOR_GRAY_DARK, COLOR_GRAY_LIGHT
)
import icons

# ==============================================================================
# CONFIGURACIÓN DE PINES FÍSICOS DEL RP2040 (Raspberry Pi Pico)
# ==============================================================================
# - UART RX:
#     Pin Físico 2  -> GPIO 1 (UART0 RX)
#     Pin Físico 1  -> GPIO 0 (UART0 TX - opcional/respuestas)
#
# - PANTALLA TFT ILI9341 (SPI0):
#     Pin Físico 22 -> GPIO 17 (CS - Chip Select)
#     Pin Físico 24 -> GPIO 18 (SCK - Serial Clock)
#     Pin Físico 25 -> GPIO 19 (MOSI / SDI - Master Out Slave In)
#     Pin Físico 26 -> GPIO 20 (DC - Data/Command)
#     Pin Físico 27 -> GPIO 21 (RST - Reset)
#     Pin Físico 29 -> GPIO 22 (LED - Control de luz de fondo / Backlight)
#
# - ALIMENTACIÓN:
#     Pin Físico 23 o 28 -> GND (Tierra de la pantalla)
#     Pin Físico 36      -> 3V3 (VCC si módulo es de 3.3V)
# ==============================================================================

UART_RX_GPIO = 1    # Pin Físico 2
UART_TX_GPIO = 0    # Pin Físico 1
UART_BAUDRATE = 115200

# Rotación de pantalla: 1 = Paisaje normal (320x240), 3 = Paisaje invertido 180°
DEFAULT_ROTATION = 3

SPI_CS_GPIO  = 17   # Pin Físico 22
SPI_SCK_GPIO = 18   # Pin Físico 24
SPI_MOSI_GPIO= 19   # Pin Físico 25
SPI_DC_GPIO  = 20   # Pin Físico 26
SPI_RST_GPIO = 21   # Pin Físico 27
SPI_BL_GPIO  = 22   # Pin Físico 29

# ==============================================================================
# ESTADO DEL SISTEMA (9 VARIABLES)
# ==============================================================================
system_state = {
    # Sensores analógicos / numéricos
    "t_amb": 24.0,   # Temperatura ambiente (DHT11) en °C
    "h_amb": 55.0,   # Humedad ambiente (DHT11) en %
    "t_prod": 15.0,  # Temp. producto (LM35) en °C
    "t_dis": 32.0,   # Temp. del disipador en °C
    # Actuadores (Estados ON/OFF)
    "dis": False,    # Encendido del disipador
    "pel": False,    # Encendido de la celda Peltier
    "rad": False,    # Encendido del radiador
    "v_des": False,  # Encendido ventilador de deshumidificador
    "hum": False,    # Encendido de los humidificadores
}

# Copia de respaldo para refresco diferencial (evita parpadeos en pantalla)
prev_state = {k: None for k in system_state}

# Animación (alterna cuadros 0 y 1 para los actuadores activos)
anim_frame = 0
last_anim_time = 0
last_rx_time = 0
demo_mode = True   # Si no se recibe UART en 5s, inicia demostración automática

# ==============================================================================
# DEFINICIÓN DE TARJETAS EN EL DISPLAY (3x3 = 9 ELEMENTOS)
# ==============================================================================
# Coordenadas y dimensiones de cada tarjeta en 320x240
# Ancho: 98px, Alto: 66px, Margen superior: 28px
CARDS_CONFIG = [
    # Fila 0
    {"key": "t_amb",  "col": 0, "row": 0, "title": "T. AMBIENTE",  "unit": "°C", "is_bool": False},
    {"key": "h_amb",  "col": 1, "row": 0, "title": "HUM. AMB.",    "unit": "%",  "is_bool": False},
    {"key": "t_prod", "col": 2, "row": 0, "title": "T. PRODUCTO", "unit": "°C", "is_bool": False},
    # Fila 1
    {"key": "t_dis",  "col": 0, "row": 1, "title": "T. DISIPADOR","unit": "°C", "is_bool": False},
    {"key": "dis",    "col": 1, "row": 1, "title": "DISIPADOR",   "unit": "",   "is_bool": True},
    {"key": "pel",    "col": 2, "row": 1, "title": "PELTIER",     "unit": "",   "is_bool": True},
    # Fila 2
    {"key": "rad",    "col": 0, "row": 2, "title": "RADIADOR",    "unit": "",   "is_bool": True},
    {"key": "v_des",  "col": 1, "row": 2, "title": "V. DESHUM.",  "unit": "",   "is_bool": True},
    {"key": "hum",    "col": 2, "row": 2, "title": "HUMIDIFIC.",  "unit": "",   "is_bool": True},
]

def get_card_rect(col, row):
    x = 6 + col * (98 + 7)  # col 0: 6, col 1: 111, col 2: 216
    y = 28 + row * (66 + 4) # row 0: 28, row 1: 98, row 2: 168
    return x, y, 98, 66

# ==============================================================================
# INICIALIZACIÓN DE HARDWARE
# ==============================================================================
print("Iniciando bus SPI para ILI9341...")
# Hardware SPI0 a 31.25 MHz (altísima velocidad y nitidez en RP2040)
spi = machine.SPI(0, baudrate=31_250_000, polarity=0, phase=0,
                  sck=machine.Pin(SPI_SCK_GPIO),
                  mosi=machine.Pin(SPI_MOSI_GPIO))

tft = ILI9341(
    spi=spi,
    cs_pin=SPI_CS_GPIO,
    dc_pin=SPI_DC_GPIO,
    rst_pin=SPI_RST_GPIO,
    bl_pin=SPI_BL_GPIO,
    width=320,
    height=240,
    rotation=DEFAULT_ROTATION # Modo horizontal (paisaje)
)

print(f"Iniciando UART0 en Pin Físico 2 (GP{UART_RX_GPIO}) a {UART_BAUDRATE} baudios...")
uart = machine.UART(0, baudrate=UART_BAUDRATE,
                    tx=machine.Pin(UART_TX_GPIO),
                    rx=machine.Pin(UART_RX_GPIO))

# ==============================================================================
# FUNCIONES GRÁFICAS DE ALTO NIVEL
# ==============================================================================

def draw_header():
    """Dibuja la barra de título superior y marco decorativo."""
    tft.fill_rect(0, 0, 320, 24, COLOR_HEADER_BG)
    tft.draw_hline(0, 24, 320, COLOR_CYAN)
    tft.draw_text("SISTEMA DE CONTROL CLIMATICO", 50, 8, COLOR_WHITE, COLOR_HEADER_BG, scale=1)
    # Etiqueta de estado UART
    tft.draw_text("RX", 285, 8, COLOR_GRAY_LIGHT, COLOR_HEADER_BG, scale=1)
    tft.fill_rect(302, 10, 6, 6, COLOR_GRAY_DARK)

def indicate_rx_activity():
    """Destello verde en el indicador RX superior cuando llega un comando."""
    tft.fill_rect(302, 10, 6, 6, COLOR_GREEN)

def clear_rx_activity():
    tft.fill_rect(302, 10, 6, 6, COLOR_GRAY_DARK)

def draw_card_frame(col, row, title):
    """Dibuja la estructura fija de una tarjeta (marco y título)."""
    x, y, w, h = get_card_rect(col, row)
    # Fondo y borde
    tft.fill_rect(x, y, w, h, COLOR_CARD_BG)
    tft.draw_rect(x, y, w, h, COLOR_CARD_BORDER)
    # Título en la parte superior
    tft.draw_text(title, x + 6, y + 5, COLOR_CYAN, COLOR_CARD_BG, scale=1)
    # Separador sutil
    tft.draw_hline(x + 4, y + 16, w - 8, COLOR_CARD_BORDER)

def render_icon_and_value(card_cfg, frame_toggle):
    """Refresca el icono ilustrativo y el valor/estado sin parpadeos."""
    key = card_cfg["key"]
    val = system_state[key]
    x, y, w, h = get_card_rect(card_cfg["col"], card_cfg["row"])
    
    icon_x = x + 6
    icon_y = y + 26
    val_x  = x + 28
    val_y  = y + 26
    
    # --------------------------------------------------------------------------
    # 1. ICONO ILUSTRATIVO QUE CAMBIA DINÁMICAMENTE
    # --------------------------------------------------------------------------
    # Limpia únicamente el recuadro del icono (16x16)
    tft.fill_rect(icon_x, icon_y, 16, 16, COLOR_CARD_BG)
    
    if key == "t_amb":
        # Termómetro DHT11: azul (<20°C), verde (20-30°C), rojo (>30°C)
        mercury_color = COLOR_CYAN if val < 20.0 else (COLOR_GREEN if val <= 30.0 else COLOR_RED)
        palette = {'.': None, '#': COLOR_WHITE, 'x': mercury_color}
        tft.draw_bitmap_16x16(icons.ICON_THERMO_EMPTY, icon_x, icon_y, palette)
        
    elif key == "h_amb":
        # Gota DHT11: ámbar (<40%), cian ideal (40-70%), azul profundo (>70%)
        water_color = COLOR_AMBER if val < 40.0 else (COLOR_CYAN if val <= 70.0 else COLOR_BLUE)
        palette = {'.': None, '#': COLOR_WHITE, 'x': water_color, 'o': COLOR_WHITE}
        tft.draw_bitmap_16x16(icons.ICON_DROPLET, icon_x, icon_y, palette)
        
    elif key == "t_prod":
        # Producto LM35: sonda de color según frío/adecuado/calor
        prod_color = COLOR_CYAN if val < 8.0 else (COLOR_GREEN if val <= 18.0 else COLOR_AMBER)
        palette = {'.': None, '#': COLOR_WHITE, 'x': COLOR_GRAY_LIGHT, 'o': prod_color}
        tft.draw_bitmap_16x16(icons.ICON_PRODUCT, icon_x, icon_y, palette)
        
    elif key == "t_dis":
        # Disipador: aletas verdes (<40°C), ámbar (40-60°C), rojo con ondas (>60°C)
        heat_color = COLOR_GREEN if val < 40.0 else (COLOR_AMBER if val <= 60.0 else COLOR_RED)
        wave_color = heat_color if val >= 50.0 else None
        palette = {'.': None, '#': heat_color, 'x': wave_color}
        tft.draw_bitmap_16x16(icons.ICON_HEATSINK, icon_x, icon_y, palette)
        
    elif key == "dis":
        # Ventilador Disipador: aspas apagadas (gris) o animadas girando (verde)
        if val:
            frame = icons.ICON_FAN_F1 if frame_toggle else icons.ICON_FAN_F0
            palette = {'.': None, '#': COLOR_GREEN}
        else:
            frame = icons.ICON_FAN_F0
            palette = {'.': None, '#': COLOR_GRAY_DARK}
        tft.draw_bitmap_16x16(frame, icon_x, icon_y, palette)
        
    elif key == "pel":
        # Célula Peltier: copo apagado o cristal brillante de hielo
        if val:
            palette = {'.': None, '#': COLOR_CYAN, 'x': COLOR_WHITE}
        else:
            palette = {'.': None, '#': COLOR_GRAY_DARK, 'x': None}
        tft.draw_bitmap_16x16(icons.ICON_PELTIER, icon_x, icon_y, palette)
        
    elif key == "rad":
        # Radiador: serpentín apagado o incandescente naranja
        if val:
            palette = {'.': None, '#': COLOR_RED, 'x': COLOR_AMBER}
        else:
            palette = {'.': None, '#': COLOR_GRAY_DARK, 'x': None}
        tft.draw_bitmap_16x16(icons.ICON_RADIATOR, icon_x, icon_y, palette)
        
    elif key == "v_des":
        # Ventilador Deshumidificador: aspas animadas en magenta/púrpura
        if val:
            frame = icons.ICON_DEHUM_FAN_F1 if frame_toggle else icons.ICON_DEHUM_FAN_F0
            palette = {'.': None, '#': COLOR_PURPLE}
        else:
            frame = icons.ICON_DEHUM_FAN_F0
            palette = {'.': None, '#': COLOR_GRAY_DARK}
        tft.draw_bitmap_16x16(frame, icon_x, icon_y, palette)
        
    elif key == "hum":
        # Humidificador: boquilla con vapor/niebla ascendente animada en azul celeste
        if val:
            frame = icons.ICON_HUMIDIFIER_F1 if frame_toggle else icons.ICON_HUMIDIFIER_F0
            palette = {'.': None, '#': COLOR_WHITE, 'x': COLOR_CYAN}
        else:
            frame = icons.ICON_HUMIDIFIER_F0
            palette = {'.': None, '#': COLOR_GRAY_DARK, 'x': None}
        tft.draw_bitmap_16x16(frame, icon_x, icon_y, palette)

    # --------------------------------------------------------------------------
    # 2. ESTADO O VALOR A MOSTRAR
    # --------------------------------------------------------------------------
    # Limpia el área del valor numérico / badge (64x28)
    tft.fill_rect(val_x, val_y - 2, 64, 28, COLOR_CARD_BG)
    
    if card_cfg["is_bool"]:
        # Insignia (badge) tipo píldora para ON / OFF
        badge_w = 60
        badge_h = 20
        badge_bg = COLOR_GREEN if val else COLOR_GRAY_DARK
        text_color = COLOR_BLACK if val else COLOR_WHITE
        badge_text = "ENC" if val else "APA"
        
        tft.fill_rect(val_x + 2, val_y + 2, badge_w, badge_h, badge_bg)
        # Centrar texto en badge: ancho 3 letras * 6px = 18px -> (60 - 18)/2 = 21px
        tft.draw_text(badge_text, val_x + 23, val_y + 8, text_color, badge_bg, scale=1)
    else:
        # Valor numérico con formato (ej: "24.5 °C")
        val_str = f"{val:4.1f}"
        unit_str = card_cfg["unit"]
        
        # Color del valor acorde a la magnitud
        if key in ("t_amb", "t_prod", "t_dis"):
            txt_color = COLOR_CYAN if val < 20 else (COLOR_GREEN if val <= 35 else COLOR_RED)
        else: # Humedad
            txt_color = COLOR_CYAN
            
        tft.draw_text(val_str, val_x + 2, val_y + 4, txt_color, COLOR_CARD_BG, scale=1)
        tft.draw_text(unit_str, val_x + 36, val_y + 16, COLOR_GRAY_LIGHT, COLOR_CARD_BG, scale=1)

# ==============================================================================
# PROCESAMIENTO DE COMANDOS UART (CLAVE-VALOR)
# ==============================================================================

# Diccionario de alias flexibles para los comandos recibidos
KEY_ALIASES = {
    "T_AMB": "t_amb", "TAMB": "t_amb", "TEMP_AMB": "t_amb", "DHT_T": "t_amb",
    "H_AMB": "h_amb", "HAMB": "h_amb", "HUM_AMB": "h_amb", "DHT_H": "h_amb",
    "T_PROD": "t_prod", "TPROD": "t_prod", "TEMP_PROD": "t_prod", "LM35": "t_prod",
    "T_DIS": "t_dis", "TDIS": "t_dis", "TEMP_DIS": "t_dis", "DISIPADOR_TEMP": "t_dis",
    "DIS": "dis", "DISIPADOR": "dis", "FAN_DIS": "dis",
    "PEL": "pel", "PELTIER": "pel", "CELDA": "pel",
    "RAD": "rad", "RADIADOR": "rad", "HEATER": "rad",
    "V_DES": "v_des", "VDES": "v_des", "VENT_DES": "v_des", "DESHUM": "v_des",
    "HUM": "hum", "HUMIDIFICADOR": "hum", "HUMIDIFICADORES": "hum",
}

def parse_uart_command(line_str):
    """
    Procesa tramas UART en formato clave-valor.
    Acepta comandos individuales o múltiples separados por comas o punto y coma:
      Ejemplos:
        T_AMB=25.4
        PEL=ON
        T_AMB=24.1, H_AMB=60.5, RAD=ON, DIS=0
    """
    global demo_mode, last_rx_time
    # Al recibir el primer comando real, se desactiva el modo demo
    demo_mode = False
    last_rx_time = time.ticks_ms()
    indicate_rx_activity()
    
    # Separar pares por coma o punto y coma
    segments = line_str.replace(";", ",").split(",")
    for seg in segments:
        seg = seg.strip()
        if not seg or "=" not in seg:
            continue
        raw_key, raw_val = seg.split("=", 1)
        key = raw_key.strip().upper()
        val = raw_val.strip().upper()
        
        # Comando especial para cambiar rotación sobre la marcha (ej: ROT=1 o ROT=3)
        if key in ("ROT", "ROTACION", "ROTATION"):
            try:
                new_rot = int(val)
                print(f"Cambiando rotación a {new_rot}...")
                tft.set_rotation(new_rot)
                tft.clear_ram(0x0000)
                tft.fill_screen(COLOR_NAVY_DARK)
                draw_header()
                for cfg in CARDS_CONFIG:
                    draw_card_frame(cfg["col"], cfg["row"], cfg["title"])
                    render_icon_and_value(cfg, anim_frame)
                    prev_state[cfg["key"]] = system_state[cfg["key"]]
            except:
                pass
            continue

        # Inversión de color en caliente (INV=1 o INV=0)
        if key in ("INV", "INVERSION"):
            inv_state = val in ("1", "ON", "TRUE")
            print(f"Inversión de color: {inv_state}")
            tft.set_inversion(inv_state)
            continue

        # Alternar orden de color BGR / RGB (BGR=1 o BGR=0)
        if key in ("BGR", "RGB"):
            bgr_state = (key == "BGR" and val in ("1", "ON", "TRUE")) or (key == "RGB" and val in ("0", "OFF", "FALSE"))
            print(f"Modo BGR: {bgr_state}")
            tft.set_rotation(tft.rotation, bgr=bgr_state)
            tft.fill_screen(COLOR_NAVY_DARK)
            draw_header()
            for cfg in CARDS_CONFIG:
                draw_card_frame(cfg["col"], cfg["row"], cfg["title"])
                render_icon_and_value(cfg, anim_frame)
                prev_state[cfg["key"]] = system_state[cfg["key"]]
            continue

        target_key = KEY_ALIASES.get(key)
        if not target_key:
            continue
            
        try:
            if target_key in ("dis", "pel", "rad", "v_des", "hum"):
                # Interpretación de estados ON/OFF
                system_state[target_key] = val in ("ON", "1", "TRUE", "SI", "HIGH", "ACTIVO")
            else:
                # Interpretación numérica float
                system_state[target_key] = float(val)
        except ValueError:
            pass

# ==============================================================================
# SECUENCIA DE ARRANQUE Y BUCLE PRINCIPAL
# ==============================================================================

def main():
    global anim_frame, last_anim_time, demo_mode
    
    # 1. Limpieza de toda la memoria interna de la pantalla (320x320) y dibujo inicial
    tft.clear_ram(0x0000)
    tft.fill_screen(COLOR_NAVY_DARK)
    draw_header()
    
    for cfg in CARDS_CONFIG:
        draw_card_frame(cfg["col"], cfg["row"], cfg["title"])
        render_icon_and_value(cfg, anim_frame)
        prev_state[cfg["key"]] = system_state[cfg["key"]]
        
    print("Sistema listo. Esperando comandos UART en Pin Físico 2 (GP1)...")
    
    uart_buffer = b""
    demo_counter = 0
    rx_led_timeout = 0
    
    # Monitor de entrada para USB CDC (para pruebas desde consola/Thonny/PC)
    stdin_poll = select.poll()
    stdin_poll.register(sys.stdin, select.POLLIN)

    while True:
        current_time = time.ticks_ms()
        
        # ----------------------------------------------------------------------
        # A. LECTURA NO BLOQUEANTE DE UART (Pin Físico 2 / GP1)
        # ----------------------------------------------------------------------
        if uart.any():
            incoming = uart.read()
            if incoming:
                uart_buffer += incoming
                rx_led_timeout = current_time + 150 # Destello LED RX
                
        # Lectura no bloqueante por USB CDC (Consola / PC)
        if stdin_poll.poll(0):
            char_in = sys.stdin.read(1)
            if char_in:
                uart_buffer += char_in.encode("utf-8")
                rx_led_timeout = current_time + 150

        # Procesar líneas completas recibidas
        while b"\n" in uart_buffer or b"\r" in uart_buffer:
            if b"\n" in uart_buffer:
                line, uart_buffer = uart_buffer.split(b"\n", 1)
            else:
                line, uart_buffer = uart_buffer.split(b"\r", 1)
                
            line_decoded = line.decode("utf-8", "ignore").strip()
            if line_decoded:
                print(f"[COMANDO RX]: {line_decoded}")
                parse_uart_command(line_decoded)
                        
        if rx_led_timeout and current_time > rx_led_timeout:
            clear_rx_activity()
            rx_led_timeout = 0

        # ----------------------------------------------------------------------
        # B. MODO DEMO / SIMULACIÓN (Si no hay UART conectado en los primeros 5s)
        # ----------------------------------------------------------------------
        if demo_mode and (current_time > 5000):
            demo_counter += 1
            if demo_counter % 20 == 0:
                # Variar suavemente las temperaturas y alternar actuadores
                system_state["t_amb"] = 23.5 + (demo_counter % 10) * 0.4
                system_state["h_amb"] = 50.0 + (demo_counter % 15) * 1.2
                system_state["t_prod"] = 12.0 + (demo_counter % 8) * 0.5
                system_state["t_dis"] = 35.0 + (demo_counter % 12) * 1.5
                system_state["dis"] = ((demo_counter // 20) % 2) == 1
                system_state["pel"] = ((demo_counter // 40) % 2) == 1
                system_state["rad"] = ((demo_counter // 60) % 2) == 1
                system_state["v_des"] = ((demo_counter // 30) % 2) == 1
                system_state["hum"] = ((demo_counter // 50) % 2) == 1

        # ----------------------------------------------------------------------
        # C. ANIMACIÓN Y ACTUALIZACIÓN VISUAL DE TARJETAS
        # ----------------------------------------------------------------------
        # Alterna cuadro de animación cada 400ms para ventiladores y niebla
        update_animation = False
        if time.ticks_diff(current_time, last_anim_time) > 400:
            anim_frame = 1 if anim_frame == 0 else 0
            last_anim_time = current_time
            update_animation = True

        for cfg in CARDS_CONFIG:
            key = cfg["key"]
            val = system_state[key]
            
            # Redibuja si el valor cambió o si está activo y requiere animación
            needs_redraw = (val != prev_state[key])
            if cfg["is_bool"] and val and update_animation:
                needs_redraw = True
                
            if needs_redraw:
                render_icon_and_value(cfg, anim_frame)
                prev_state[key] = val
                
        time.sleep_ms(30)

if __name__ == "__main__":
    main()
