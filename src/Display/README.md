# Sistema de Monitoreo de 9 Variables en Pantalla TFT ILI9341 con RP2040

Este proyecto implementa un tablero gráfico en tiempo real para una pantalla TFT ILI9341 (320x240) controlada por un microcontrolador **RP2040** (Raspberry Pi Pico) programado en **MicroPython**.

El sistema recibe lecturas y estados a través de su receptor **UART (Pin Físico 2)** en formato clave-valor y actualiza las 9 variables con texto descriptivo, valores en tiempo real e **iconos dinámicos animados** que cambian según el estado o magnitud de cada variable.

---

## 1. Tabla de Conexiones (Pines Físicos vs GPIO)

> **Nota:** Todos los pines enumerados corresponden a los **pines físicos del módulo RP2040 / Raspberry Pi Pico** (formato de 40 pines), tal como fue solicitado.

| Función / Señal | Pin Físico RP2040 | GPIO MicroPython | Conexión en Pantalla ILI9341 / Emisor | Notas |
| :--- | :---: | :---: | :--- | :--- |
| **UART0 RX** | **Pin 2** | `GPIO 1` | **TX** del microcontrolador emisor / sensor | Recepción de tramas UART |
| **UART0 TX** | Pin 1 | `GPIO 0` | (Opcional) RX del emisor | Salida UART / debug |
| **GND UART** | Pin 3, 23 o 28 | `GND` | **GND** del emisor | Masa común obligatoria |
| **SPI0 CS** | **Pin 22** | `GPIO 17` | **CS** (Chip Select) | Selección de pantalla SPI |
| **SPI0 SCK** | **Pin 24** | `GPIO 18` | **SCK / CLK** | Reloj SPI (31.25 MHz) |
| **SPI0 MOSI** | **Pin 25** | `GPIO 19` | **MOSI / SDI** | Entrada de datos pantalla |
| **TFT DC** | **Pin 26** | `GPIO 20` | **DC / RS** (Data/Command) | Control Comando / Datos |
| **TFT RESET** | **Pin 27** | `GPIO 21` | **RESET / RST** | Reset de pantalla |
| **TFT LED** | **Pin 29** | `GPIO 22` | **LED / BL** (Backlight) | Encendido de luz de fondo |
| **GND Pantalla**| Pin 23 o 28 | `GND` | **GND** | Tierra de alimentación |
| **VCC Pantalla**| Pin 36 o 40 | `3V3` o `VBUS` | **VCC** | 3.3V (Pin 36) o 5V (Pin 40 con regulador) |

---

## 2. Las 9 Variables y su Comportamiento Visual

La pantalla organiza las 9 variables en una cuadrícula de 3x3 tarjetas de alto contraste con refresco diferencial (cero parpadeo):

| # | Variable | Clave UART | Tipo | Texto en Pantalla | Icono Dinámico / Ilustración que cambia |
| :-: | :--- | :--- | :-: | :--- | :--- |
| **1** | **Temperatura Ambiental** (DHT11) | `T_AMB` | Numérica (°C) | `T. AMBIENTE` | **Termómetro:** Nivel de mercurio cambia: Azul (<20°C), Verde (20-30°C), Rojo (>30°C). |
| **2** | **Humedad Ambiental** (DHT11) | `H_AMB` | Numérica (%) | `HUM. AMB.` | **Gota de agua:** Ámbar seco (<40%), Cian ideal (40-70%), Azul saturado (>70%). |
| **3** | **Temperatura de Producto** (LM35) | `T_PROD` | Numérica (°C) | `T. PRODUCTO` | **Caja con sonda:** Sonda térmica cambia a Cian (frío <8°C), Verde (óptimo 8-18°C), Ámbar (>18°C). |
| **4** | **Temperatura del Disipador** | `T_DIS` | Numérica (°C) | `T. DISIPADOR` | **Disipador:** Aletas Verdes (<40°C), Ámbar (40-60°C), Rojo con ondas de calor activas (>60°C). |
| **5** | **Encendido del Disipador** | `DIS` | ON / OFF | `DISIPADOR` | **Ventilador:** Gris apagado cuando OFF. Aspas en verde brillante rotando/girando cuando ON. |
| **6** | **Encendido de la Peltier** | `PEL` | ON / OFF | `PELTIER` | **Célula Peltier:** Gris apagado cuando OFF. Copo de cristal cian resplandeciente con destellos cuando ON. |
| **7** | **Encendido del Radiador** | `RAD` | ON / OFF | `RADIADOR` | **Radiador:** Serpentín gris apagado cuando OFF. Serpentín incandescente naranja/rojo radiando calor cuando ON. |
| **8** | **Ventilador Deshumidificador** | `V_DES` | ON / OFF | `V. DESHUM.` | **Turbina extractora:** Gris apagado cuando OFF. Turbina activa magenta girando cuando ON. |
| **9** | **Humidificadores** | `HUM` | ON / OFF | `HUMIDIFIC.` | **Boquilla vapor:** Gris sin vapor cuando OFF. Boquilla con nubes de niebla/vapor azul celeste ascendiendo cuando ON. |

---

## 3. Protocolo de Comandos UART (Pin Físico 2)

El sistema procesa texto en formato clave-valor sin importar mayúsculas o minúsculas.

### Valores admitidos:
- **Variables analógicas:** Números enteros o decimales (ej: `24.5`, `60`, `14.2`).
- **Actuadores ON/OFF:** `ON`, `OFF`, `1`, `0`, `TRUE`, `FALSE`, `SI`, `NO`, `HIGH`, `LOW`.

### Ejemplos de tramas:

#### A. Comandos individuales (uno por línea):
```text
T_AMB=24.5
H_AMB=62.0
T_PROD=15.3
T_DIS=38.4
DIS=ON
PEL=ON
RAD=OFF
V_DES=OFF
HUM=ON
```

#### B. Múltiples comandos en una sola línea (separados por coma o punto y coma):
```text
T_AMB=25.2, H_AMB=55.0, PEL=1, DIS=1, RAD=0
```
```text
T_PROD=12.5; T_DIS=41.2; V_DES=ON; HUM=OFF
```

#### C. Alias reconocidos automáticamente:
- `T_AMB`, `TAMB`, `TEMP_AMB`, `DHT_T`
- `H_AMB`, `HAMB`, `HUM_AMB`, `DHT_H`
- `T_PROD`, `TPROD`, `TEMP_PROD`, `LM35`
- `T_DIS`, `TDIS`, `TEMP_DIS`
- `DIS`, `DISIPADOR`, `FAN_DIS`
- `PEL`, `PELTIER`, `CELDA`
- `RAD`, `RADIADOR`, `HEATER`
- `V_DES`, `VDES`, `VENT_DES`, `DESHUM`
- `HUM`, `HUMIDIFICADOR`, `HUMIDIFICADORES`

---

## 4. Estructura de Archivos

```
c:\Users\migue\Display\
│
├── ili9341.py           # Controlador de pantalla SPI de alta velocidad para RP2040
├── icons.py             # Mapas de bits 16x16 de iconos dinámicos y cuadros de animación
├── main.py              # Programa principal (UART, lógica, refresco gráfico)
├── test_uart_sender.py  # Script para enviar datos de prueba desde PC vía USB-Serial
└── README.md            # Este manual de referencia
```

---

## 5. Puesta en Marcha Rápida con Thonny

1. Conecta tu **Raspberry Pi Pico (RP2040)** a la computadora con el botón **BOOTSEL** presionado y carga el firmware oficial de **MicroPython**.
2. Abre el IDE [Thonny](https://thonny.org/).
3. Selecciona en la esquina inferior derecha el intérprete **MicroPython (Raspberry Pi Pico)**.
4. Copia los archivos `ili9341.py`, `icons.py` y `main.py` a la memoria interna de la Raspberry Pi Pico.
5. Ejecuta `main.py`.
   - **Modo Demostración Automático:** Si no hay ningún emisor UART conectado en los primeros 5 segundos, la pantalla comenzará una animación de demostración variando las temperaturas y encendiendo/apagando los actuadores para que verifiques de inmediato el cableado.
   - En cuanto envíes el primer comando por el **Pin Físico 2**, el modo demo se desactiva y pasa al control en vivo.
