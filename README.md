# Sistema de Control Ambiental Inteligente y Monitoreo IoT

Sistema embebido industrial/IoT para control de climatización, temperatura y humedad con arquitectura distribuida de doble microcontrolador: **ESP32-S3** (núcleo de control en tiempo real con FreeRTOS y ESP-IDF) y **RP2040** (módulo gráfico dedicado para pantalla TFT ILI9341 con MicroPython).

---

## 📌 Arquitectura del Sistema

El proyecto opera bajo un esquema maestro-esclavo desacoplado:

```
                      +-----------------------------------+
                      |      Sensores de Entrada          |
                      |  - DHT11 (Temp/Hum Ambiente)      |
                      |  - LM35 #1 (Temp Disipador)       |
                      |  - LM35 #2 (Temp Producto)        |
                      +-----------------+-----------------+
                                        |
                                        v
+------------------+          +-------------------+          +-------------------+
|  Consola de      |  UART0   |     ESP32-S3      |  UART1   |  RP2040 Pico      |
|  Usuario (CLI)   |<-------->|  (FreeRTOS Core)  |--------->|  (MicroPython)    |
|  PC / COM17      | 115200   +---------+---------+ 115200   +---------+---------+
+------------------+                    |                              |
                                        v                              v
                              +-------------------+          +-------------------+
                              | Módulo de Relés   |          | Pantalla TFT      |
                              | (Actuadores 1-5)  |          | ILI9341 (320x240) |
                              | - Celda Peltier   |          | 9 Variables con   |
                              | - Disipador       |          | Iconos Dinámicos  |
                              | - Calefactor      |          +-------------------+
                              | - Deshumidificador|
                              | - Humidificador   |
                              +-------------------+
```

---

## 🚀 Características Principales

- **Control Térmico y de Humedad con Histéresis:**
  - Banda muerta de temperatura: $\pm 0.5\,^\circ\text{C}$
  - Banda muerta de humedad: $\pm 2.0\,\%$
- **Seguridad y Protección Activa de Hardware:**
  - Monitoreo continuo de la temperatura del disipador de la celda Peltier.
  - Corte de emergencia automático si $T_{\text{disipador}} > 50.0\,^\circ\text{C}$ (apaga la celda inmediatamente y fuerza la ventilación de alivio hasta que la temperatura baje a $45.0\,^\circ\text{C}$).
- **Multitarea con FreeRTOS (ESP32-S3):**
  - `sensors_task`: Muestreo de sensores DHT11 y ADC calibrados (eFuse + oversampling x16 para LM35).
  - `control_task`: Máquina de estados de climatización y control de relés (*Active LOW*).
  - `display_task`: Formateo y transmisión periódica de tramas UART (1 Hz).
  - `cli_task`: Consola interactiva por puerto serial para ajuste dinámico de consignas.
- **Tablero Gráfico en Tiempo Real (RP2040 + TFT ILI9341):**
  - Matriz de 3x3 tarjetas de alto contraste con refresco diferencial (sin parpadeo).
  - 9 variables en vivo con animaciones e iconos dinámicos según magnitudes.
  - Modo demostración (*Demo Mode*) automático en ausencia de señal UART.

---

## 🔌 Asignación de Pines (Pinout)

### 1. ESP32-S3 (Controlador Principal)

| Periférico / Función | Pin GPIO | Descripción |
| :--- | :---: | :--- |
| **Relé 1 (DIS)** | `GPIO 35` | Ventilación / Refrigeración del disipador Peltier |
| **Relé 2 (PEL)** | `GPIO 36` | Alimentación de la Celda Peltier |
| **Relé 3 (RAD)** | `GPIO 37` | Calefactor / Radiador térmico |
| **Relé 4 (V_DES)** | `GPIO 38` | Ventilador extractor para deshumidificación |
| **Relé 5 (HUM)** | `GPIO 39` | Atomizadores ultrasónicos (Humidificadores) |
| **DHT11 DATA** | `GPIO 40` | Bus digital de temperatura y humedad ambiente |
| **LM35 Disipador** | `GPIO 9` | ADC1 Canal 8 (Lectura analógica con calibración) |
| **LM35 Producto** | `GPIO 11` | ADC2 Canal 0 (Lectura analógica con calibración) |
| **UART Display TX** | `GPIO 17` | Transmisión serial hacia el Pin Físico 2 del RP2040 |
| **UART Display RX** | `GPIO 18` | Recepción serial (opcional/debug) |
| **UART Consola (CLI)** | `GPIO 43/44` | UART0 a través del puerto USB-Serial (115200 baud) |

> **Nota sobre los relés:** Operan con lógica activa en bajo (*Active LOW*): `0 = ON`, `1 = OFF`.

---

### 2. RP2040 (Pantalla TFT ILI9341 - MicroPython)

| Función | Pin Físico RP2040 | GPIO MicroPython | Conexión en Pantalla / ESP32-S3 |
| :--- | :---: | :---: | :--- |
| **UART RX** | **Pin 2** | `GPIO 1` | `GPIO 17` (TX) del ESP32-S3 |
| **SPI CS** | **Pin 22** | `GPIO 17` | **CS** Pantalla ILI9341 |
| **SPI SCK** | **Pin 24** | `GPIO 18` | **SCK / CLK** Pantalla |
| **SPI MOSI** | **Pin 25** | `GPIO 19` | **MOSI / SDI** Pantalla |
| **TFT DC** | **Pin 26** | `GPIO 20` | **DC / RS** Pantalla |
| **TFT RESET**| **Pin 27** | `GPIO 21` | **RST** Pantalla |
| **TFT LED** | **Pin 29** | `GPIO 22` | **LED / BL** (Backlight) |
| **GND Común**| **Pin 3, 23 o 28** | `GND` | GND común con ESP32-S3 y Pantalla |

---

## 📊 Las 9 Variables Monitoreadas

| # | Clave UART | Variable | Tipo | Comportamiento Visual en Pantalla |
| :-: | :--- | :--- | :-: | :--- |
| **1** | `T_AMB` | Temperatura Ambiente | Numérica (°C) | Termómetro dinámico: Azul (<20°C), Verde (20-30°C), Rojo (>30°C) |
| **2** | `H_AMB` | Humedad Ambiente | Numérica (%) | Gota de agua: Ámbar (<40%), Cian ideal (40-70%), Azul (>70%) |
| **3** | `T_PROD` | Temperatura Producto | Numérica (°C) | Sonda térmica: Cian (<8°C), Verde (8-18°C), Ámbar (>18°C) |
| **4** | `T_DIS` | Temperatura Disipador | Numérica (°C) | Disipador: Verde (<40°C), Ámbar (40-60°C), Rojo alerta (>60°C) |
| **5** | `DIS` | Ventilador Disipador | ON / OFF | Aspas giratorias en verde brillante al activarse |
| **6** | `PEL` | Celda Peltier | ON / OFF | Copo de cristal cian resplandeciente al encender |
| **7** | `RAD` | Radiador / Calefactor | ON / OFF | Serpentín incandescente con radiación de calor |
| **8** | `V_DES` | Deshumidificador | ON / OFF | Turbina extractora animada magenta |
| **9** | `HUM` | Humidificador | ON / OFF | Boquilla vaporizadora con nubes ascendentes celestes |

---

## 💻 Consola Serial Interactiva (CLI)

Al conectar el ESP32-S3 a la computadora por el puerto serial (por defecto `COM17` a 115200 baudios), se dispone de una interfaz de comandos:

### Comandos de Ajuste de Consignas:
- `T:28` : Fija la temperatura objetivo a $28.0\,^\circ\text{C}$ y activa el control térmico.
- `H:60` : Fija la humedad objetivo al $60.0\,\%$ y activa el control de humedad.
- `T:24, H:55` o `H:55, T:24` : Ajusta simultáneamente ambas consignas.

### Comandos de Diagnóstico y Control:
- `STATUS` : Muestra un informe completo en tabla con las lecturas analógicas, estado de relés y alertas de seguridad.
- `STOP` : Detiene de inmediato el lazo de control y apaga todos los actuadores.
- `HELP` : Muestra la guía rápida de comandos en pantalla.

---

## 📁 Estructura del Repositorio

```
.
├── platformio.ini              # Configuración de compilación para ESP32-S3 en PlatformIO
├── CMakeLists.txt              # Configuración raíz de CMake para ESP-IDF
├── sdkconfig.defaults          # Opciones predeterminadas de ESP-IDF
├── src/
│   ├── main.c                  # Núcleo del firmware ESP32-S3 (FreeRTOS, ADC, Relés, CLI)
│   ├── dht11.c / dht11.h       # Driver para sensor digital DHT11
│   ├── ds1307.c / ds1307.h     # Driver para RTC I2C DS1307
│   ├── hcsr04.c / hcsr04.h     # Driver para sensor ultrasónico HC-SR04
│   ├── ssd1306.c / ssd1306.h   # Driver y fuentes para pantalla OLED I2C secundaria
│   ├── Comands.md              # Especificación del protocolo serial de tramas
│   └── Display/                # Código fuente del módulo de visualización (RP2040)
│       ├── main.py             # Script principal en MicroPython (Receptor UART y GUI)
│       ├── ili9341.py          # Controlador SPI optimizado para ILI9341
│       ├── icons.py            # Bitmaps y mapas de animación (16x16)
│       ├── test_uart_sender.py # Utilidad de pruebas seriales desde PC
│       └── README.md           # Guía de conexión y puesta en marcha del display
└── README.md                   # Documentación general del proyecto
```

---

## 🛠️ Instalación y Compilación

### Requisitos
1. [Visual Studio Code](https://code.visualstudio.com/) con la extensión [PlatformIO IDE](https://platformio.org/).
2. [Thonny IDE](https://thonny.org/) (o extensión de MicroPython) para programar la Raspberry Pi Pico.

### 1. Cargar el Firmware en el ESP32-S3
1. Abre esta carpeta en VS Code con PlatformIO.
2. Verifica el puerto COM asignado en `platformio.ini` (por defecto `COM17`):
   ```ini
   upload_port = COM17
   monitor_port = COM17
   ```
3. Compila y flashea el proyecto:
   ```bash
   pio run --target upload
   ```
4. Abre el monitor serie para interactuar con la consola:
   ```bash
   pio device monitor
   ```

### 2. Configurar la Pantalla en el RP2040
1. Conecta la Raspberry Pi Pico manteniendo pulsado el botón **BOOTSEL** y carga el firmware de MicroPython.
2. Abre Thonny y selecciona el intérprete **MicroPython (Raspberry Pi Pico)**.
3. Copia a la raíz de la placa los archivos ubicados en `src/Display/`:
   - `ili9341.py`
   - `icons.py`
   - `main.py`
4. Reinicia la placa o ejecuta `main.py`.

---

## 👥 Autores y Colaboradores

- **AlejoZapata** - [GitHub Profile](https://github.com/AlejoZapata)
- Proyecto desarrollado como solución de control e instrumentación ambiental IoT.
