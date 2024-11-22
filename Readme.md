# Aurivox: Sistema de Procesamiento de Audio Multibanda

## 🎧 Descripción
Aurivox es un sistema de procesamiento de audio en tiempo real basado en ESP32 que implementa un compresor de rango dinámico multibanda (WDRC) similar al utilizado en audífonos protésicos modernos. El sistema procesa el audio digitalmente utilizando FFT para separar la señal en múltiples bandas frecuenciales y aplicar compresión específica a cada banda.

```
Diagrama del Sistema:
                                                
INMP441 → ESP32 (WDRC) → PCM5102A
   ↓           ↓            ↓
Micrófono   Proceso      Salida
   I2S      Digital       I2S
```

## 🔧 Hardware Necesario

### Componentes Principales:
- ESP32 (WROOM-DA)
- Micrófono INMP441 (I2S)
- DAC PCM5102A (I2S)

### Conexiones
#### INMP441 (Micrófono):
```
VDD  → 3.3V
GND  → GND
SD   → GPIO33 (DATA)
WS   → GPIO25 (LRCK)
SCK  → GPIO32 (BCLK)
L/R  → GND
```

#### PCM5102A (DAC):
```
VCC  → 5V/3.3V
GND  → GND
SCK  → GPIO14 (BCLK)
DIN  → GPIO26 (DATA)
LCK  → GPIO27 (LRCK)
FMT  → GND
FLT  → GND
DMP  → GND
SCL  → GND
XMT  → 3.3V
```

## 💻 Estructura del Software

### Archivos del Proyecto:
```
Aurivox/
├── Aurivox.ino          # Programa principal
├── config.h             # Configuraciones y constantes
├── wdrc.h              # Clase WDRC (header)
├── wdrc.cpp            # Implementación WDRC
├── multiband_wdrc.h    # Clase MultibandWDRC (header)
├── multiband_wdrc.cpp  # Implementación MultibandWDRC
├── i2s_handler.h       # Funciones I2S (header)
└── i2s_handler.cpp     # Implementación I2S
```

### Características Técnicas:
- Frecuencia de muestreo: 44.1 kHz
- Resolución: 32 bits
- Tamaño FFT: 512 puntos
- Bandas de frecuencia: 3
  - Baja: 250-1000 Hz
  - Media: 1000-4000 Hz
  - Alta: 4000-8000 Hz

### Procesamiento por Bandas:
```
Banda    | Frecuencia | Compresión | Ganancia | Uso
---------|------------|------------|----------|----------------
Baja     | 250-1k Hz |    2:1     |  15 dB   | Graves/Voces
Media    | 1k-4k Hz  |    3:1     |  10 dB   | Vocales
Alta     | 4k-8k Hz  |    4:1     |   5 dB   | Consonantes
```

## 📊 Parámetros WDRC por Banda

### Banda Baja (250-1000 Hz):
- Threshold: -50 dB
- Ratio: 2:1
- Attack: 10ms
- Release: 100ms
- Knee: 10 dB
- Ganancia: 15 dB

### Banda Media (1000-4000 Hz):
- Threshold: -45 dB
- Ratio: 3:1
- Attack: 5ms
- Release: 50ms
- Knee: 8 dB
- Ganancia: 10 dB

### Banda Alta (4000-8000 Hz):
- Threshold: -40 dB
- Ratio: 4:1
- Attack: 3ms
- Release: 25ms
- Knee: 6 dB
- Ganancia: 5 dB

## 🛠️ Instalación

1. **Preparación del IDE:**
   - Instalar Arduino IDE
   - Agregar soporte ESP32
   - Instalar librería ArduinoFFT

2. **Conexiones Hardware:**
   - Realizar las conexiones del INMP441 y PCM5102A según el diagrama
   - Verificar alimentación y tierras

3. **Carga del Software:**
   - Clonar el repositorio
   - Abrir Aurivox.ino en Arduino IDE
   - Seleccionar placa "ESP32 WROOM-DA"
   - Compilar y cargar

## 📝 Monitoreo y Depuración

El sistema incluye monitoreo en tiempo real a través del puerto serie:
- FPS de procesamiento
- Conteo de errores
- Estadísticas por banda
- Uso de memoria

Para monitorear:
```bash
# Linux/Mac
screen /dev/ttyUSB0 115200

# Windows
# Usar Monitor Serie de Arduino IDE
```

## 🔍 Diagnóstico

### Indicadores LED:
- LED BUILTIN parpadea: Sistema funcionando
- LED rápido: Error de I2S
- LED lento: Procesando audio

### Problemas Comunes:
1. No hay sonido:
   - Verificar conexiones I2S
   - Comprobar alimentación del DAC
   
2. Distorsión:
   - Reducir ganancia de entrada
   - Ajustar thresholds WDRC

## 📖 Referencias

- [ESP32 I2S Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/i2s.html)
- [WDRC en Audífonos](https://www.who.int/publications/i/item/9789241549127)
- [FFT Processing](https://www.analog.com/media/en/technical-documentation/dsp-book/dsp_book_Ch31.pdf)

## 🤝 Contribuciones

Las contribuciones son bienvenidas. Por favor:
1. Fork del repositorio
2. Crear rama feature
3. Commit cambios
4. Push a la rama
5. Crear Pull Request

## 📄 Licencia

Este proyecto está bajo la Licencia MIT. Ver archivo `LICENSE` para detalles.