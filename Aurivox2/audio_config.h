// ==================== AUDIO_CONFIG.H ====================
// Configuraciones globales, constantes y estructuras para Aurivox v3.0
// Archivo compartido por todos los módulos del sistema

#ifndef AUDIO_CONFIG_H
#define AUDIO_CONFIG_H

#include "driver/i2s.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// ==================== VERSIÓN DEL SISTEMA ====================

#define AURIVOX_VERSION_MAJOR   3
#define AURIVOX_VERSION_MINOR   0
#define AURIVOX_VERSION_PATCH   0
#define AURIVOX_VERSION_STRING  "3.0.0"

// ==================== CONFIGURACIONES DE HARDWARE ====================

// Pines del micrófono ICS-43434
#define I2S_MIC_BCLK    2   // D2 - Bit Clock
#define I2S_MIC_LRCL    4   // D4 - Left/Right Clock (Word Select)
#define I2S_MIC_DOUT    5   // D5 - Data Output del micrófono

// Pines del DAC MAX98357A
#define I2S_DAC_BCLK    6   // D6 - Bit Clock
#define I2S_DAC_LRC     7   // D7 - Left/Right Clock (Word Select)
#define I2S_DAC_DIN     8   // D8 - Data Input al DAC

// Pines de botones
#define BTN_GAIN_UP     3   // D3 - Botón subir ganancia
#define BTN_GAIN_DOWN   0   // D0 - Botón bajar ganancia  
#define BTN_SLEEP       1   // D1 - Botón sleep/wake (mantener 3s)

// ==================== CONFIGURACIONES DE AUDIO ====================

// Parámetros básicos de audio (definidos como variables en audio_config.cpp)
// Estos valores están disponibles como extern variables, no como #define
#define LATENCY_TARGET_MS   25  // Latencia objetivo en ms

// Puertos I2S (definidos como variables en audio_config.cpp)
// Estos valores están disponibles como extern variables, no como #define

// Tipos de datos de audio
typedef int32_t mic_sample_t;   // Micrófono: 32-bit
typedef int16_t dac_sample_t;   // DAC: 16-bit
typedef float   dsp_sample_t;   // Procesamiento DSP: float

// ==================== CONFIGURACIONES DE BOTONES ====================

// Timing de botones
#define DEBOUNCE_DELAY      50    // 50ms debounce
#define SLEEP_HOLD_TIME     3000  // 3 segundos para activar sleep

// ==================== CONFIGURACIONES DEL SISTEMA DE PIPS ====================

#define PIP_FREQUENCY       1000  // 1kHz para pips
#define PIP_AMPLITUDE       0.5   // 50% amplitud para pips
#define PIP_DURATION_MS     200   // 200ms por pip
#define PIP_GAP_MS          100   // 100ms entre pips
#define PI                  3.14159265359

// Calcular samples por pip (usando variables extern)
// Nota: PIP_SAMPLES se calculará en runtime ya que SAMPLE_RATE es variable
extern const int PIP_SAMPLES;

// ==================== CONFIGURACIONES DSP ====================

// Número de bandas del ecualizador
#define EQ_BANDS_COUNT      6

// Frecuencias centrales del ecualizador (Hz)
#define EQ_FREQ_250HZ       250.0f
#define EQ_FREQ_500HZ       500.0f
#define EQ_FREQ_1KHZ        1000.0f
#define EQ_FREQ_2KHZ        2000.0f
#define EQ_FREQ_4KHZ        4000.0f
#define EQ_FREQ_8KHZ        8000.0f

// Límites de ganancia
#define GAIN_LEVELS_COUNT   5
#define EQ_GAIN_MIN_DB      -20.0f
#define EQ_GAIN_MAX_DB      +20.0f

// Configuraciones WDRC
#define WDRC_THRESHOLD_MIN_DB   -60.0f
#define WDRC_THRESHOLD_MAX_DB   0.0f
#define WDRC_RATIO_MIN          1.0f
#define WDRC_RATIO_MAX          10.0f
#define WDRC_ATTACK_MIN_MS      1.0f
#define WDRC_ATTACK_MAX_MS      1000.0f
#define WDRC_RELEASE_MIN_MS     10.0f
#define WDRC_RELEASE_MAX_MS     5000.0f

// ==================== CONFIGURACIONES DE MEMORIA ====================

// Configuraciones NVS
#define NVS_NAMESPACE       "audio_config"
#define CONFIG_VERSION      1
#define MAX_PRESET_NAME     32

// Límites de memoria
#define MIN_FREE_HEAP       50000   // RAM mínima requerida (50KB)
#define STACK_SIZE_AUDIO    4096    // Stack para tarea de audio (4KB)
#define STACK_SIZE_CONTROL  4096    // Stack para tarea de control (4KB)

// ==================== PRIORIDADES DE TAREAS ====================

#define PRIORITY_AUDIO_TASK     2   // Prioridad alta para audio
#define PRIORITY_CONTROL_TASK   1   // Prioridad menor para control

// ==================== ESTRUCTURAS DE DATOS ====================

// Estructura para configuración de filtro pasa-altos
struct HighpassConfig {
  bool enabled;
  float cutoff_freq;
  float alpha;        // Coeficiente calculado
  float prev_input;   // Estado anterior
  float prev_output;  // Estado anterior
};

// Estructura para banda del ecualizador
struct EQBand {
  bool enabled;
  float freq;          // Frecuencia central
  float gain_db;       // Ganancia en dB
  float gain_linear;   // Ganancia lineal
  float Q;             // Factor de calidad
  // Coeficientes del filtro biquad
  float b0, b1, b2;    // Numerador
  float a1, a2;        // Denominador (a0 = 1)
  // Estados del filtro
  float x1, x2;        // Entradas anteriores
  float y1, y2;        // Salidas anteriores
};

// Estructura para ecualizador completo
struct EqualizerConfig {
  bool enabled;
  EQBand bands[EQ_BANDS_COUNT];
};

// Estructura para WDRC
struct WDRCConfig {
  bool enabled;
  float threshold_db;  // Umbral de compresión
  float ratio;         // Ratio de compresión
  float attack_ms;     // Tiempo de attack
  float release_ms;    // Tiempo de release
  // Estados internos (calculados)
  float envelope;      // Envolvente del detector
  float gain_reduction; // Reducción de ganancia actual
};

// Estructura para limitador
struct LimiterConfig {
  bool enabled;
  float threshold_db;  // Umbral del limitador
  float attack_ms;     // Tiempo de attack
  float release_ms;    // Tiempo de release
  // Estados internos
  float envelope;      // Envolvente del detector
  float gain_reduction; // Reducción de ganancia
};

// Estructura principal de configuración
struct AudioConfig {
  uint32_t version;
  
  // Ganancia general
  int gain_level;
  
  // Filtro pasa-altos
  bool highpass_enabled;
  float highpass_freq;
  
  // Ecualizador
  bool eq_enabled;
  float eq_gains[EQ_BANDS_COUNT];
  
  // WDRC (compresión)
  bool wdrc_enabled;
  float wdrc_threshold;
  float wdrc_ratio;
  float wdrc_attack;
  float wdrc_release;
  
  // Limitador
  bool limiter_enabled;
  float limiter_threshold;
  
  // Bluetooth (futuro)
  bool bluetooth_enabled;
  bool cross_mode_enabled;
  
  // Checksum para integridad
  uint32_t checksum;
};

// ==================== SISTEMA DE PIPS ====================

struct PipSystem {
  bool active;
  int total_pips;
  int remaining_pips;
  int samples_in_current_pip;
  unsigned long pip_start_time;
  unsigned long pip_gap_start;
  bool in_gap;
  float phase;
};

// ==================== ENUMERACIONES ====================

// Estados del sistema
enum SystemState {
  SYSTEM_INITIALIZING,
  SYSTEM_ACTIVE,
  SYSTEM_SLEEPING,
  SYSTEM_ERROR
};

// Tipos de preset
enum PresetType {
  PRESET_DEFAULT,
  PRESET_MILD_LOSS,
  PRESET_MODERATE_LOSS,
  PRESET_SEVERE_LOSS,
  PRESET_MUSIC,
  PRESET_SPEECH,
  PRESET_CUSTOM
};

// Modos de conectividad (futuro)
enum ConnectivityMode {
  MODE_STANDALONE,
  MODE_CROSS,
  MODE_BICROSS,
  MODE_BLUETOOTH_ONLY
};

// ==================== CONSTANTES GLOBALES ====================

// Niveles de ganancia predefinidos
extern const float GAIN_LEVELS[GAIN_LEVELS_COUNT];
extern const float gain_levels[5];  // Para compatibilidad con código existente

// Frecuencias del ecualizador
extern const float EQ_FREQUENCIES[EQ_BANDS_COUNT];

// Configuración por defecto
extern const AudioConfig DEFAULT_CONFIG;

// Variables de hardware (definidas en audio_config.cpp)
extern const int SAMPLE_RATE;      // 16000 Hz
extern const int BUFFER_SIZE;      // 128 muestras
extern const i2s_port_t I2S_PORT_MIC;  // I2S_NUM_0
extern const i2s_port_t I2S_PORT_DAC;   // I2S_NUM_1
extern const int PIP_SAMPLES;      // Calculado: (SAMPLE_RATE * PIP_DURATION_MS) / 1000

// ==================== MACROS ÚTILES ====================

// Conversión entre dB y lineal
#define DB_TO_LINEAR(db)    (pow(10.0f, (db) / 20.0f))
#define LINEAR_TO_DB(lin)   (20.0f * log10f(lin))

// Limitación de valores
#define CLAMP(val, min, max) ((val) < (min) ? (min) : ((val) > (max) ? (max) : (val)))

// Conversión de samples (usando variables extern)
extern float ms_to_samples(float ms);
extern float samples_to_ms(int samples);

// ==================== VERIFICACIONES DE COMPILACIÓN ====================

// Verificar que el buffer no cause latencia excesiva
#if (BUFFER_SIZE * 1000 / SAMPLE_RATE) > LATENCY_TARGET_MS
#warning "Buffer size may cause excessive latency"
#endif

// Verificar configuración de memoria
#if STACK_SIZE_AUDIO < 2048
#error "Audio task stack size too small"
#endif

#if STACK_SIZE_CONTROL < 2048
#error "Control task stack size too small"
#endif

// ==================== INFORMACIÓN DE COMPILACIÓN ====================

// Para debugging y diagnóstico
#define COMPILE_DATE        __DATE__
#define COMPILE_TIME        __TIME__
#define COMPILER_VERSION    __VERSION__

// ==================== COMENTARIOS DE DESARROLLO ====================

/*
 * ESTRUCTURA DEL PIPELINE DSP PLANIFICADO:
 * 
 * Micrófono (32-bit) → 
 * ↓
 * Conversión a float →
 * ↓  
 * Filtro Pasa-Altos (ESP-DSP) →
 * ↓
 * Ecualizador 6 Bandas (ESP-DSP) →
 * ↓
 * WDRC (Compresión Dinámica) →
 * ↓
 * Limitador Anti-Clipping →
 * ↓
 * Ganancia Final →
 * ↓
 * Conversión a 16-bit → DAC
 * 
 * LATENCIA OBJETIVO: < 25ms total
 * CPU TARGET: < 70% Core 0, < 30% Core 1
 * MEMORIA TARGET: < 200KB total
 */

#endif // AUDIO_CONFIG_H