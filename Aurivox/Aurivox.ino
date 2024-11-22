#include "config.h"
#include "multiband_wdrc.h"
#include "i2s_handler.h"

/*
 * SISTEMA DE PROCESAMIENTO DE AUDIO MULTIBAND WDRC
 * ==============================================
 *
 * Diagrama General del Sistema:
 * ===========================
 *
 *   INMP441        Procesamiento Digital          PCM5102A
 *  ┌───────┐      ┌──────────────────┐          ┌────────┐
 *  │ MIC   │ I2S  │  Multiband WDRC  │   I2S    │  DAC   │
 *  │   ◊===┼────▶│    FFT + DSP     ├────────▶│   ♪    │
 *  └───────┘      └──────────────────┘          └────────┘
 *
 * 
 * Diagrama de Procesamiento de Señal:
 * =================================
 *
 *     Input    ┌─────────┐   ┌───────────┐   ┌──────┐    Output
 * ─────────────┤   FFT   ├───┤ Split Band├───┤ IFFT ├──────────▶ 
 *              └─────────┘   └─────┬─────┘   └──────┘
 *                                  │
 *                           ┌──────┴─────┐
 *                           │  3 bandas  │
 *                           │    WDRC    │
 *                           └────────────┘
 * 
 * Diagrama de Bandas Frecuenciales:
 * ===============================
 * 
 * Ganancia (dB)
 *    ▲                    
 *  15├──────┐
 *    │      │
 *  10├      └────┐
 *    │           │
 *   5├           └──────
 *    │
 *   0├───┬───┬───┬───┬───▶ Freq (Hz)
 *    0  250  1k  4k  8k
 *       Baja  Media  Alta
 *
 * 
 * Parámetros WDRC por Banda:
 * ========================
 * 
 *  Nivel de Salida (dB)             Banda Baja (250-1000 Hz)
 *   ↑                               - Mayor ganancia (15dB)
 *   │        /                      - Compresión suave (2:1)
 *   │     /··                       - Ataque lento
 *   │  /·                       
 *   │/                          Banda Media (1k-4k Hz)
 *   │                           - Ganancia media (10dB)
 *   └────────────────→         - Compresión media (3:1)
 *        Nivel de              - Ataque medio
 *        Entrada (dB)       
 *                           Banda Alta (4k-8k Hz)
 *                           - Menor ganancia (5dB)
 *                           - Compresión fuerte (4:1)
 *                           - Ataque rápido
 *
 * Procesamiento por Muestra:
 * ========================
 * 
 *  [Sample] → [Window] → [FFT] → [Band Split] → [WDRC] → [IFFT] → [Sample]
 *     ↓          ↓         ↓          ↓           ↓         ↓         ↓
 *    32b    Hamming   Spectrum   3 Bandas    Compress   Time    Stereo Out
 */

//================================================
// VARIABLES GLOBALES
//================================================

MultibandWDRC multiband_wdrc;
float buffer_proc[BUFFER_SIZE];

//================================================
// DEBUG Y MONITOREO
//================================================

#define MONITOR_INTERVAL 1000  // Intervalo de monitoreo en ms
unsigned long last_monitor = 0;
uint32_t process_count = 0;
uint32_t error_count = 0;

void monitor_performance() {
    unsigned long current_time = millis();
    if (current_time - last_monitor >= MONITOR_INTERVAL) {
        float fps = process_count * 1000.0f / MONITOR_INTERVAL;
        Serial.printf("FPS: %.2f, Errores: %lu\n", fps, error_count);
        process_count = 0;
        error_count = 0;
        last_monitor = current_time;
    }
}

//================================================
// FUNCIONES PRINCIPALES
//================================================

void setup() {
    Serial.begin(115200);
    
    setup_i2s_mic();
    setup_i2s_dac();
    
    Serial.println("\n=================================");
    Serial.println("Sistema de audio multibanda iniciado");
    Serial.println("Configuración:");
    Serial.printf("Sample Rate: %d Hz\n", SAMPLE_RATE);
    Serial.printf("Buffer Size: %d muestras\n", BUFFER_SIZE);
    Serial.printf("Bandas: %d\n", NUM_BANDS);
    Serial.println("Límites de bandas (Hz):");
    for(int i = 0; i < NUM_BANDS; i++) {
        Serial.printf("Banda %d: %.0f - %.0f Hz\n", 
                     i, BAND_LIMITS[i], BAND_LIMITS[i+1]);
    }
    Serial.println("=================================\n");
}

void loop() {
    int32_t samples_in[BUFFER_SIZE];
    int32_t samples_out[BUFFER_SIZE * 2];
    size_t bytes_read = 0;
    size_t bytes_written = 0;
    
    // Leer muestras del micrófono
    esp_err_t read_result = i2s_read(I2S_NUM_0, samples_in, 
                                    sizeof(samples_in), &bytes_read, 
                                    portMAX_DELAY);
                                    
    if (read_result != ESP_OK) {
        error_count++;
        return;
    }
    
    // Convertir muestras a float [-1,1]
    for(int i = 0; i < BUFFER_SIZE; i++) {
        buffer_proc[i] = (float)samples_in[i] / INT32_MAX;
    }
    
    // Procesar con WDRC multibanda
    multiband_wdrc.process(buffer_proc, buffer_proc, BUFFER_SIZE);
    
    // Convertir de vuelta a int32 y duplicar para estéreo
    for(int i = 0; i < BUFFER_SIZE; i++) {
        int32_t processed = (int32_t)(buffer_proc[i] * INT32_MAX);
        samples_out[i*2] = processed;      // Canal izquierdo
        samples_out[i*2+1] = processed;    // Canal derecho
    }
    
    // Enviar al DAC
    esp_err_t write_result = i2s_write(I2S_NUM_1, samples_out, 
                                      bytes_read * 2, &bytes_written, 
                                      portMAX_DELAY);
                                      
    if (write_result != ESP_OK) {
        error_count++;
        return;
    }
    
    process_count++;
    monitor_performance();
}