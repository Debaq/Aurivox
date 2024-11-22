#include "wdrc.h"

/*
 * WIDE DYNAMIC RANGE COMPRESSION (WDRC)
 * ===================================
 *
 * El WDRC es un sistema de compresión de audio que ajusta dinámicamente
 * la ganancia basándose en el nivel de entrada. Su funcionamiento es similar
 * al del oído humano y es fundamental en audífonos protésicos.
 *
 * Diagrama de Bloques del WDRC:
 * ===========================
 *
 *  Input    Detector de   Cálculo de    Aplicación     Output
 *  ─────╮   Envolvente    Ganancia     de Ganancia    ╭─────▶
 *       │        ↓            ↓             ↓         │
 *       v    ┌─────┐     ┌──────┐      ┌─────┐      v
 *    ──────▶│ RMS ├───▶│Compr.├────▶│ Gain├──────────▶
 *            └─────┘     └──────┘      └─────┘
 *               ↑            ↑            ↑
 *           Attack/      Threshold     Limiter
 *           Release       Ratio
 *
 * Curva de Compresión:
 * ==================
 *
 *  Nivel de Salida (dB)
 *       ↑                                  
 *    0  +···························/     → Sin compresión (1:1)
 *       |                        /·/
 *  -10  +                    /·/··
 *       |                 /·/           → Knee suave
 *  -20  +             /·/    
 *       |         /·/              → Zona de compresión (ratio:1)
 *  -30  +     /·/                    
 *       | /·/
 *  -40  +/                          → Threshold
 *       |___+____+____+____+____+→    Nivel de Entrada (dB)
 *     -60   -50   -40  -30   -20   
 *
 * Detector de Envolvente:
 * =====================
 * 
 *  Nivel     Attack Time →│  │← Release Time
 *    ↑          ┌────┐    │  │    ┌────
 *    │          │    │    │  │    │
 *    │     ┌────┘    │    │  │    │
 *    │     │         └────┘  └────┘
 *    │     │              
 *    └─────┴───────────────────────→ Tiempo
 *
 */

// Constructor: inicializa el detector de envolvente
WDRC::WDRC() : envelope(0.0f) {
    /* 
     * El envelope se inicia en 0 para partir del silencio
     * y evitar artefactos al inicio del procesamiento
     */
}

// Configuración de parámetros para cada banda
void WDRC::setParameters(const BandParams& params) {
    /*
     * Parámetros de compresión:
     * ------------------------
     *  threshold:  Nivel donde comienza la compresión (dB)
     *  ratio:      Relación de compresión (N:1)
     *  knee_width: Suavidad de la transición (dB)
     *  band_gain:  Ganancia adicional de la banda (dB)
     *
     * Cálculo de coeficientes del detector de envolvente:
     * -----------------------------------------------
     *              1
     * alpha = 1 - ---
     *            τ·fs
     *
     * Donde:
     * τ = tiempo de ataque/liberación (s)
     * fs = frecuencia de muestreo (Hz)
     */
    
    threshold = params.threshold;
    ratio = params.ratio;
    knee_width = params.knee_width;
    band_gain = params.gain;
    
    // Cálculo de coeficientes temporales
    alpha_attack = exp(-1.0f / (SAMPLE_RATE * params.attack_time));
    alpha_release = exp(-1.0f / (SAMPLE_RATE * params.release_time));
}

// Conversión de decibelios a escala lineal
float WDRC::db_to_linear(float db) {
    /*
     * Conversión dB a lineal:
     * ----------------------
     *           dB/20
     * gain = 10
     *
     *  dB  | Lineal
     * -----+--------
     *    0 |   1.0
     *    6 |   2.0
     *  -6  |   0.5
     *  -20 |   0.1
     */
    return pow(10.0f, db / 20.0f);
}

// Conversión de escala lineal a decibelios
float WDRC::linear_to_db(float linear) {
    /*
     * Conversión lineal a dB:
     * ----------------------
     *              linear
     * dB = 20 · log10(---)
     *               ref
     *
     * Se añade un pequeño offset (1e-9) para evitar log(0)
     */
    return 20.0f * log10(fabs(linear) + 1e-9f);
}

// Procesamiento de una muestra
float WDRC::process(float input) {
    /*
     * Proceso de compresión WDRC:
     * =========================
     *
     * 1. Conversión a dB
     * 2. Detección de envolvente
     * 3. Cálculo de ganancia
     * 4. Aplicación de ganancia
     * 5. Limitación de salida
     *
     * Diagrama de estados del detector de envolvente:
     * -------------------------------------------
     *
     *           input > env
     *      ┌────────────────┐
     *      │                v
     *    Release ←───── Envelope ────→ Attack
     *      ^                │
     *      └────────────────┘
     *           input < env
     */

    // 1. Convertir entrada a dB
    float input_db = linear_to_db(input);
    
    // 2. Actualizar detector de envolvente
    if (input_db > envelope) {
        // Fase de ataque (nivel aumentando)
        envelope = alpha_attack * envelope + (1.0f - alpha_attack) * input_db;
    } else {
        // Fase de liberación (nivel disminuyendo)
        envelope = alpha_release * envelope + (1.0f - alpha_release) * input_db;
    }
    
    // 3. Calcular ganancia de compresión
    float gain_db = 0.0f;
    float diff = envelope - threshold;
    
    /*
     * Zonas de compresión:
     * ------------------
     *
     *    Ganancia
     *       ↑
     *       │    Knee
     *   ────┼──○○○○
     *       │      ○○○○
     *       │          ○○○○  Compresión
     *       │              ○○○○
     *   ────┼────┼────┼────┼───→ Nivel
     *       │    │    │    │
     *           t-w  t   t+w    t = threshold
     *                           w = knee_width
     */
    
    if (fabs(diff) <= knee_width/2) {
        // Región de la rodilla (transición suave)
        float knee_factor = diff + knee_width/2;
        gain_db = (ratio - 1) * knee_factor * knee_factor / (2 * knee_width);
    } else if (diff > knee_width/2) {
        // Región de compresión completa
        gain_db = (ratio - 1) * diff;
    }
    
    // 4. Aplicar ganancia de compresión y ganancia de banda
    float output = input * db_to_linear(-gain_db + band_gain);
    
    // 5. Limitar salida para evitar distorsión
    if (output > 0.99f) output = 0.99f;
    if (output < -0.99f) output = -0.99f;
    
    return output;
}