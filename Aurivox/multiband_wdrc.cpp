#include "multiband_wdrc.h"

/*
 * PROCESAMIENTO MULTIBANDA CON FFT Y WDRC
 * ======================================
 *
 * Este sistema implementa un procesador de audio multibanda utilizando
 * la Transformada Rápida de Fourier (FFT) para separar el audio en
 * diferentes bandas frecuenciales y aplicar WDRC específico a cada una.
 *
 * Diagrama del Sistema Multibanda:
 * ==============================
 *
 * Input   ┌─────┐ ┌───────────────────────────┐ ┌─────┐  Output
 * ────────│ FFT │─│ Procesamiento Frecuencial │─│IFFT │─────────▶
 *         └─────┘ └───────────────────────────┘ └─────┘
 *                           ▲   ▲   ▲
 *                           │   │   │
 *                     ┌─────┴───┴───┴─────┐
 *                     │  División Bandas  │
 *                     └─────────────────┬─┘
 *                           │   │   │
 *                     ┌─────┴───┴───┴─────┐
 *                     │    WDRC x Banda   │
 *                     └─────────────────┬─┘
 *
 * Procesamiento en el Dominio de la Frecuencia:
 * ==========================================
 *
 * Magnitud (dB)        Bandas de Frecuencia
 *     ↑          B1         B2         B3
 *     │     ┌─────────┐┌─────────┐┌─────────┐
 *     │     │         ││         ││         │
 *     │     │  WDRC1  ││  WDRC2  ││  WDRC3  │
 *     │     │         ││         ││         │
 *     │     └─────────┘└─────────┘└─────────┘
 *     └──────┬─────────┬─────────┬─────────→ Freq (Hz)
 *           250       1k        4k         8k
 *
 * Proceso de Ventaneo (Windowing):
 * ==============================
 *
 * Amplitud          Ventana Hamming
 *    ↑     ___________________________
 *1.0 │   ╱╲                    
 *    │  ╱   ╲        Reduce el
 *    │ ╱     ╲     "spectral leakage"
 *    │╱       ╲    en el análisis FFT
 *    └─────────────────────────→ Muestras
 *    0                        511
 *
 * Manejo de Fase y Magnitud:
 * ========================
 *
 *            FFT
 * Señal ──────────────▶ Mag∠Phase
 *   │                      │
 *   │       Procesado      │
 *   │     ┌───────────┐    │
 *   │     │ Mag * G   │    │
 *   │     └───────────┘    │
 *   │           ↓          ↓
 *   │     Nueva Mag    Phase
 *   │           ↓          ↓
 *   └───────────────────────
 *            IFFT
 */

// Constructor: inicializa FFT y WDRC para cada banda
MultibandWDRC::MultibandWDRC() {
    /*
     * Inicialización:
     * -------------
     * - FFT para análisis espectral
     * - Array de procesadores WDRC
     * - Buffers para datos reales e imaginarios
     *
     * Configuración FFT:
     * ----------------
     * - Tamaño: 512 puntos
     * - Frecuencia muestreo: 44100 Hz
     * - Resolución: 44100/512 = 86.13 Hz/bin
     */
    FFT = new ArduinoFFT<double>(real, imag, FFT_SIZE, (double)SAMPLE_RATE);
    
    // Inicializar cada banda con sus parámetros específicos
    for(int i = 0; i < NUM_BANDS; i++) {
        wdrc_bands[i].setParameters(BAND_PARAMS[i]);
    }
}

// Destructor: libera memoria
MultibandWDRC::~MultibandWDRC() {
    if (FFT != nullptr) {
        delete FFT;
    }
}

// Determina a qué banda pertenece cada frecuencia
int MultibandWDRC::getBandIndex(double frequency) {
    /*
     * Mapeo de Frecuencias a Bandas:
     * ===========================
     *
     * Banda │ Rango (Hz) │ Relevancia
     * ──────┼───────────┼────────────
     *   0   │ 250-1000  │ Graves, fundamentales
     *   1   │ 1000-4000 │ Medios, vocales
     *   2   │ 4000-8000 │ Agudos, consonantes
     */
    for(int i = 0; i < NUM_BANDS; i++) {
        if(frequency >= BAND_LIMITS[i] && frequency < BAND_LIMITS[i + 1]) {
            return i;
        }
    }
    return -1;  // Frecuencia fuera de rango
}

// Procesamiento principal
void MultibandWDRC::process(float* input, float* output, int size) {
    /*
     * Pipeline de Procesamiento:
     * =======================
     *
     * 1. Preparación de datos
     * 2. Análisis FFT
     * 3. Procesamiento por bandas
     * 4. Síntesis IFFT
     * 5. Normalización
     */

    // 1. Preparación: copiar entrada y aplicar padding
    for(int i = 0; i < FFT_SIZE; i++) {
        real[i] = i < size ? (double)input[i] : 0.0;
        imag[i] = 0.0;
    }
    
    // 2. Análisis: ventana y FFT
    FFT->windowing(FFT_WIN_TYP_HAMMING, FFT_FORWARD);
    FFT->compute(FFT_FORWARD);
    
    // 3. Procesamiento por bandas
    for(int i = 0; i < FFT_SIZE/2; i++) {
        // Calcular frecuencia del bin actual
        double frequency = (double)i * SAMPLE_RATE / FFT_SIZE;
        int band = getBandIndex(frequency);
        
        if(band >= 0) {  // Si la frecuencia pertenece a alguna banda
            // Obtener magnitud y fase
            double mag = sqrt(real[i] * real[i] + imag[i] * imag[i]);
            double phase = atan2(imag[i], real[i]);
            
            // Aplicar WDRC de la banda correspondiente
            mag = (double)wdrc_bands[band].process((float)mag);
            
            // Reconstruir componente frecuencial
            real[i] = mag * cos(phase);
            imag[i] = mag * sin(phase);
            
            // Mantener simetría conjugada para IFFT
            if(i != 0) {
                real[FFT_SIZE - i] = real[i];
                imag[FFT_SIZE - i] = -imag[i];
            }
        }
    }
    
    // 4. Síntesis: IFFT
    FFT->compute(FFT_REVERSE);
    
    // 5. Normalización y copia a salida
    for(int i = 0; i < size; i++) {
        output[i] = (float)(real[i] / FFT_SIZE);
    }
}