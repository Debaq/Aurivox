#ifndef CONFIG_H
#define CONFIG_H

// Pines para el micrófono INMP441 (sin cambios)
#define I2S_MIC_WS    25
#define I2S_MIC_SD    33
#define I2S_MIC_SCK   32

// Pines para el MAX98357A
#define I2S_BCLK      14  // Clock
#define I2S_WCLK      27  // Word Select (LRC)
#define I2S_DOUT      26  // Data Out (DIN en MAX98357A)
#define I2S_SD_MODE   13  // Opcional: Pin de shutdown

// Configuración del sistema de audio
#define SAMPLE_RATE     44100
#define BUFFER_SIZE     512
#define DMA_BUF_COUNT   8
#define DMA_BUF_LEN     1024

// Definición de las bandas frecuenciales
#define NUM_BANDS       3
#define FFT_SIZE        512

// Límites de las bandas frecuenciales (en Hz)
const float BAND_LIMITS[NUM_BANDS + 1] = {250, 1000, 4000, 8000};

// Estructura para los parámetros de cada banda
struct BandParams {
    float threshold;    // Umbral de compresión en dB
    float ratio;        // Ratio de compresión
    float knee_width;   // Ancho de la rodilla en dB
    float gain;         // Ganancia adicional en dB
    float attack_time;  // Tiempo de ataque en segundos
    float release_time; // Tiempo de liberación en segundos
};

// Configuración específica para cada banda frecuencial
const BandParams BAND_PARAMS[NUM_BANDS] = {
    // Banda Baja (250-1000Hz)
    {-50.0f, 2.0f, 10.0f, 15.0f, 0.010f, 0.100f},
    // Banda Media (1000-4000Hz)
    {-45.0f, 3.0f, 8.0f, 10.0f, 0.005f, 0.050f},
    // Banda Alta (4000-8000Hz)
    {-40.0f, 4.0f, 6.0f, 5.0f, 0.003f, 0.025f}
};

#endif