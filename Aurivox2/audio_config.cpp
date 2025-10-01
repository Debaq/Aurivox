// ==================== AUDIO_CONFIG.CPP ====================
// Definiciones de constantes globales y configuraciones para Aurivox v3.0
// Implementación de todas las constantes declaradas en audio_config.h

#include "Arduino.h"
#include <math.h>
#include "audio_config.h"

// ==================== CONSTANTES GLOBALES REQUERIDAS ====================

// Primero undefine los macros para evitar conflictos
//#undef SAMPLE_RATE
//#undef BUFFER_SIZE
//#undef I2S_PORT_MIC  
//#undef I2S_PORT_DAC

// Ahora definir las variables que los otros archivos necesitan
//const int SAMPLE_RATE = 16000;
//const int BUFFER_SIZE = 128;
const i2s_port_t I2S_PORT_MIC = I2S_NUM_0;
const i2s_port_t I2S_PORT_DAC = I2S_NUM_1;

// Calcular PIP_SAMPLES usando las variables
const int PIP_SAMPLES = (SAMPLE_RATE * 200) / 1000;  // 200ms = PIP_DURATION_MS

// Array de ganancia que necesita button_control.cpp
const float gain_levels[5] = {0.0f, 0.25f, 0.50f, 0.75f, 1.0f};

// Funciones de conversión
float ms_to_samples(float ms) {
    return (ms * SAMPLE_RATE / 1000.0f);
}

float samples_to_ms(int samples) {
    return ((float)samples * 1000.0f / SAMPLE_RATE);
}

// Función de validación del sistema
bool validate_system_config() {
    // Verificar que el buffer no cause latencia excesiva
    float latency_ms = samples_to_ms(BUFFER_SIZE);
    if (latency_ms > 25.0f) {  // LATENCY_TARGET_MS
        Serial.printf("⚠️ Advertencia: Buffer size causa latencia de %.1fms (objetivo: 25ms)\n", latency_ms);
        return false;
    }
    
    // Verificar configuración de memoria
    if (4096 < 2048) {  // STACK_SIZE_AUDIO < 2048
        Serial.println("❌ Error: Audio task stack size muy pequeño");
        return false;
    }
    
    if (4096 < 2048) {  // STACK_SIZE_CONTROL < 2048
        Serial.println("❌ Error: Control task stack size muy pequeño");
        return false;
    }
    
    return true;
}

// ==================== NIVELES DE GANANCIA PREDEFINIDOS ====================

// Array de niveles de ganancia (5 niveles: 0%, 25%, 50%, 75%, 100%)
const float GAIN_LEVELS[GAIN_LEVELS_COUNT] = {
    0.0f,   // Nivel 1: 0% - Silencio
    0.25f,  // Nivel 2: 25% - Ganancia baja
    0.50f,  // Nivel 3: 50% - Ganancia media (default)
    0.75f,  // Nivel 4: 75% - Ganancia alta
    1.0f    // Nivel 5: 100% - Ganancia máxima
};

// ==================== FRECUENCIAS DEL ECUALIZADOR ====================

// Frecuencias centrales de las 6 bandas del ecualizador
const float EQ_FREQUENCIES[EQ_BANDS_COUNT] = {
    EQ_FREQ_250HZ,  // 250Hz - Frecuencias graves
    EQ_FREQ_500HZ,  // 500Hz - Graves medios
    EQ_FREQ_1KHZ,   // 1kHz - Medios
    EQ_FREQ_2KHZ,   // 2kHz - Medios agudos (importante para voz)
    EQ_FREQ_4KHZ,   // 4kHz - Agudos (consonantes)
    EQ_FREQ_8KHZ    // 8kHz - Agudos altos
};

// ==================== CONFIGURACIÓN POR DEFECTO ====================

// Función auxiliar para calcular checksum de configuración
static uint32_t calculate_default_checksum(const AudioConfig* config) {
    uint32_t checksum = 0;
    const uint8_t* data = (const uint8_t*)config;
    size_t size = sizeof(AudioConfig) - sizeof(uint32_t); // Excluir el checksum mismo
    
    for (size_t i = 0; i < size; i++) {
        checksum += data[i];
    }
    return checksum;
}

// Configuración por defecto del sistema
const AudioConfig DEFAULT_CONFIG = []() {
    AudioConfig config = {
        .version = CONFIG_VERSION,
        
        // Ganancia general - Nivel 3 (50%)
        .gain_level = 2,  // Índice 2 = nivel 3 (50%)
        
        // Filtro pasa-altos - Desactivado por defecto
        .highpass_enabled = false,
        .highpass_freq = 100.0f,  // 100Hz cuando se active
        
        // Ecualizador - Desactivado con todas las bandas planas
        .eq_enabled = false,
        .eq_gains = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},  // Todas las bandas en 0dB
        
        // WDRC (Wide Dynamic Range Compression) - Desactivado por defecto
        .wdrc_enabled = false,
        .wdrc_threshold = -20.0f,  // -20dB threshold típico
        .wdrc_ratio = 2.0f,        // Ratio 2:1 suave
        .wdrc_attack = 10.0f,      // 10ms attack rápido
        .wdrc_release = 100.0f,    // 100ms release medio
        
        // Limitador - Desactivado por defecto
        .limiter_enabled = false,
        .limiter_threshold = -6.0f,  // -6dB para prevenir clipping
        
        // Conectividad (futuro) - Todo desactivado por defecto
        .bluetooth_enabled = false,
        .cross_mode_enabled = false,
        
        // Checksum se calculará después
        .checksum = 0
    };
    
    // Calcular y asignar checksum
    config.checksum = calculate_default_checksum(&config);
    return config;
}();

// ==================== PRESETS PREDEFINIDOS ====================

// Preset para pérdida auditiva leve (20-40 dB)
const AudioConfig MILD_LOSS_CONFIG = []() {
    AudioConfig config = DEFAULT_CONFIG;  // Copiar base
    
    // Configuración específica para pérdida leve
    config.gain_level = 3;  // 75% ganancia
    
    // Activar filtro pasa-altos para reducir ruido
    config.highpass_enabled = true;
    config.highpass_freq = 80.0f;
    
    // Ecualizador con énfasis en frecuencias altas
    config.eq_enabled = true;
    config.eq_gains[0] = -2.0f;  // 250Hz: -2dB (reducir graves)
    config.eq_gains[1] = 0.0f;   // 500Hz: 0dB
    config.eq_gains[2] = +2.0f;  // 1kHz: +2dB (realzar medios)
    config.eq_gains[3] = +4.0f;  // 2kHz: +4dB (importante para voz)
    config.eq_gains[4] = +6.0f;  // 4kHz: +6dB (consonantes)
    config.eq_gains[5] = +3.0f;  // 8kHz: +3dB (agudos)
    
    // WDRC suave
    config.wdrc_enabled = true;
    config.wdrc_threshold = -25.0f;
    config.wdrc_ratio = 1.5f;
    
    // Recalcular checksum
    config.checksum = calculate_default_checksum(&config);
    return config;
}();

// Preset para pérdida auditiva moderada (40-60 dB)
const AudioConfig MODERATE_LOSS_CONFIG = []() {
    AudioConfig config = DEFAULT_CONFIG;
    
    config.gain_level = 4;  // 100% ganancia
    
    // Filtro pasa-altos más agresivo
    config.highpass_enabled = true;
    config.highpass_freq = 120.0f;
    
    // Ecualizador con más énfasis
    config.eq_enabled = true;
    config.eq_gains[0] = -3.0f;  // 250Hz: -3dB
    config.eq_gains[1] = +1.0f;  // 500Hz: +1dB
    config.eq_gains[2] = +5.0f;  // 1kHz: +5dB
    config.eq_gains[3] = +8.0f;  // 2kHz: +8dB
    config.eq_gains[4] = +10.0f; // 4kHz: +10dB
    config.eq_gains[5] = +6.0f;  // 8kHz: +6dB
    
    // WDRC moderado
    config.wdrc_enabled = true;
    config.wdrc_threshold = -20.0f;
    config.wdrc_ratio = 2.5f;
    
    // Limitador activo para protección
    config.limiter_enabled = true;
    config.limiter_threshold = -3.0f;
    
    config.checksum = calculate_default_checksum(&config);
    return config;
}();

// Preset para pérdida auditiva severa (60-80 dB)
const AudioConfig SEVERE_LOSS_CONFIG = []() {
    AudioConfig config = DEFAULT_CONFIG;
    
    config.gain_level = 4;  // 100% ganancia máxima
    
    // Filtro pasa-altos agresivo
    config.highpass_enabled = true;
    config.highpass_freq = 150.0f;
    
    // Ecualizador muy agresivo
    config.eq_enabled = true;
    config.eq_gains[0] = -5.0f;  // 250Hz: -5dB (cortar graves)
    config.eq_gains[1] = +2.0f;  // 500Hz: +2dB
    config.eq_gains[2] = +8.0f;  // 1kHz: +8dB
    config.eq_gains[3] = +12.0f; // 2kHz: +12dB
    config.eq_gains[4] = +15.0f; // 4kHz: +15dB (máximo énfasis)
    config.eq_gains[5] = +8.0f;  // 8kHz: +8dB
    
    // WDRC fuerte
    config.wdrc_enabled = true;
    config.wdrc_threshold = -15.0f;
    config.wdrc_ratio = 4.0f;
    config.wdrc_attack = 5.0f;   // Attack más rápido
    config.wdrc_release = 200.0f; // Release más lento
    
    // Limitador agresivo
    config.limiter_enabled = true;
    config.limiter_threshold = -1.0f;
    
    config.checksum = calculate_default_checksum(&config);
    return config;
}();

// Preset optimizado para música
const AudioConfig MUSIC_CONFIG = []() {
    AudioConfig config = DEFAULT_CONFIG;
    
    config.gain_level = 2;  // 50% ganancia para música
    
    // Sin filtro pasa-altos para preservar graves
    config.highpass_enabled = false;
    
    // Ecualizador con curva musical (smile curve suave)
    config.eq_enabled = true;
    config.eq_gains[0] = +2.0f;  // 250Hz: +2dB (graves)
    config.eq_gains[1] = +1.0f;  // 500Hz: +1dB
    config.eq_gains[2] = 0.0f;   // 1kHz: 0dB (neutro)
    config.eq_gains[3] = +1.0f;  // 2kHz: +1dB
    config.eq_gains[4] = +3.0f;  // 4kHz: +3dB (presencia)
    config.eq_gains[5] = +4.0f;  // 8kHz: +4dB (aire)
    
    // WDRC muy suave para preservar dinámica musical
    config.wdrc_enabled = true;
    config.wdrc_threshold = -30.0f;
    config.wdrc_ratio = 1.2f;
    config.wdrc_attack = 20.0f;   // Attack lento
    config.wdrc_release = 500.0f; // Release muy lento
    
    config.checksum = calculate_default_checksum(&config);
    return config;
}();

// Preset optimizado para claridad de voz
const AudioConfig SPEECH_CONFIG = []() {
    AudioConfig config = DEFAULT_CONFIG;
    
    config.gain_level = 3;  // 75% ganancia
    
    // Filtro pasa-altos para cortar ruido
    config.highpass_enabled = true;
    config.highpass_freq = 100.0f;
    
    // Ecualizador enfocado en frecuencias de voz (300Hz-3kHz)
    config.eq_enabled = true;
    config.eq_gains[0] = -2.0f;  // 250Hz: -2dB (reducir ruido)
    config.eq_gains[1] = +1.0f;  // 500Hz: +1dB (fondos vocales)
    config.eq_gains[2] = +4.0f;  // 1kHz: +4dB (claridad vocal)
    config.eq_gains[3] = +6.0f;  // 2kHz: +6dB (inteligibilidad)
    config.eq_gains[4] = +8.0f;  // 4kHz: +8dB (consonantes)
    config.eq_gains[5] = +2.0f;  // 8kHz: +2dB (sibilantes)
    
    // WDRC optimizado para voz
    config.wdrc_enabled = true;
    config.wdrc_threshold = -18.0f;
    config.wdrc_ratio = 3.0f;
    config.wdrc_attack = 8.0f;    // Attack rápido para voz
    config.wdrc_release = 150.0f; // Release medio
    
    config.checksum = calculate_default_checksum(&config);
    return config;
}();

// ==================== FUNCIONES DE UTILIDAD ====================

/**
 * @brief Obtener preset por tipo
 * 
 * @param preset_type Tipo de preset solicitado
 * @return Puntero a la configuración del preset, o nullptr si no existe
 */
const AudioConfig* get_preset_config(PresetType preset_type) {
    switch (preset_type) {
        case PRESET_DEFAULT:
            return &DEFAULT_CONFIG;
        case PRESET_MILD_LOSS:
            return &MILD_LOSS_CONFIG;
        case PRESET_MODERATE_LOSS:
            return &MODERATE_LOSS_CONFIG;
        case PRESET_SEVERE_LOSS:
            return &SEVERE_LOSS_CONFIG;
        case PRESET_MUSIC:
            return &MUSIC_CONFIG;
        case PRESET_SPEECH:
            return &SPEECH_CONFIG;
        case PRESET_CUSTOM:
        default:
            return nullptr;  // Custom preset no tiene configuración fija
    }
}

/**
 * @brief Obtener nombre del preset
 * 
 * @param preset_type Tipo de preset
 * @return Nombre legible del preset
 */
const char* get_preset_name(PresetType preset_type) {
    switch (preset_type) {
        case PRESET_DEFAULT:
            return "Default";
        case PRESET_MILD_LOSS:
            return "Pérdida Leve";
        case PRESET_MODERATE_LOSS:
            return "Pérdida Moderada";
        case PRESET_SEVERE_LOSS:
            return "Pérdida Severa";
        case PRESET_MUSIC:
            return "Música";
        case PRESET_SPEECH:
            return "Voz/Conversación";
        case PRESET_CUSTOM:
            return "Personalizado";
        default:
            return "Desconocido";
    }
}

/**
 * @brief Validar integridad de configuración
 * 
 * @param config Configuración a validar
 * @return true si la configuración es válida
 */
bool validate_audio_config(const AudioConfig* config) {
    if (!config) return false;
    
    // Verificar versión
    if (config->version != CONFIG_VERSION) {
        return false;
    }
    
    // Verificar rangos de ganancia
    if (config->gain_level < 0 || config->gain_level >= GAIN_LEVELS_COUNT) {
        return false;
    }
    
    // Verificar rangos de frecuencia
    if (config->highpass_freq < 20.0f || config->highpass_freq > 1000.0f) {
        return false;
    }
    
    // Verificar rangos del ecualizador
    for (int i = 0; i < EQ_BANDS_COUNT; i++) {
        if (config->eq_gains[i] < EQ_GAIN_MIN_DB || config->eq_gains[i] > EQ_GAIN_MAX_DB) {
            return false;
        }
    }
    
    // Verificar rangos de WDRC
    if (config->wdrc_threshold < WDRC_THRESHOLD_MIN_DB || 
        config->wdrc_threshold > WDRC_THRESHOLD_MAX_DB) {
        return false;
    }
    
    if (config->wdrc_ratio < WDRC_RATIO_MIN || config->wdrc_ratio > WDRC_RATIO_MAX) {
        return false;
    }
    
    if (config->wdrc_attack < WDRC_ATTACK_MIN_MS || 
        config->wdrc_attack > WDRC_ATTACK_MAX_MS) {
        return false;
    }
    
    if (config->wdrc_release < WDRC_RELEASE_MIN_MS || 
        config->wdrc_release > WDRC_RELEASE_MAX_MS) {
        return false;
    }
    
    // Verificar checksum
    uint32_t calculated_checksum = calculate_default_checksum(config);
    if (calculated_checksum != config->checksum) {
        return false;
    }
    
    return true;
}

/**
 * @brief Copiar configuración de forma segura
 * 
 * @param dest Configuración destino
 * @param src Configuración origen
 * @return true si la copia fue exitosa
 */
bool copy_audio_config(AudioConfig* dest, const AudioConfig* src) {
    if (!dest || !src) return false;
    
    // Validar configuración origen
    if (!validate_audio_config(src)) {
        return false;
    }
    
    // Copiar toda la estructura
    *dest = *src;
    
    return true;
}

// ==================== INFORMACIÓN DE COMPILACIÓN ====================

/**
 * @brief Obtener información de compilación
 */
const char* get_compile_info() {
    static char compile_info[256];
    snprintf(compile_info, sizeof(compile_info), 
             "Aurivox v%s - Compilado: %s %s con %s", 
             AURIVOX_VERSION_STRING, 
             COMPILE_DATE, 
             COMPILE_TIME, 
             COMPILER_VERSION);
    return compile_info;
}

/**
 * @brief Obtener información de configuración del sistema
 */
void print_system_config_info() {
    Serial.println("\n📋 INFORMACIÓN DE CONFIGURACIÓN DEL SISTEMA");
    Serial.println("═══════════════════════════════════════════════════════════");
    Serial.printf("🎧 Versión: %s\n", AURIVOX_VERSION_STRING);
    Serial.printf("📊 Sample Rate: %d Hz\n", SAMPLE_RATE);
    Serial.printf("📦 Buffer Size: %d muestras\n", BUFFER_SIZE);
    Serial.printf("⏱️ Latencia base: %.1f ms\n", (float)BUFFER_SIZE / SAMPLE_RATE * 1000);
    Serial.printf("🎚️ Niveles de ganancia: %d\n", GAIN_LEVELS_COUNT);
    Serial.printf("🎵 Bandas EQ: %d\n", EQ_BANDS_COUNT);
    Serial.printf("💾 Tamaño configuración: %d bytes\n", sizeof(AudioConfig));
    Serial.printf("🏗️ %s\n", get_compile_info());
    Serial.println("═══════════════════════════════════════════════════════════");
}