// ==================== AUDIO_HARDWARE.H ====================
// Declaraciones de funciones para manejo de hardware I2S
// Configuración y control de micrófono ICS-43434 + DAC MAX98357A

#ifndef AUDIO_HARDWARE_H
#define AUDIO_HARDWARE_H

#include "audio_config.h"
#include "esp_err.h"

// ==================== FUNCIONES PRINCIPALES ====================

/**
 * @brief Inicializar todo el hardware I2S (micrófono + DAC)
 * 
 * Configura ambos puertos I2S con los parámetros optimizados para baja latencia.
 * Debe llamarse una sola vez durante el setup del sistema.
 * 
 * @note Esta función no inicia los streams de audio, solo configura el hardware
 * @see start_audio_streams() para iniciar el flujo de audio
 */
void initialize_i2s_hardware(void);

/**
 * @brief Iniciar streams de audio en ambos puertos I2S
 * 
 * Inicia la captura del micrófono y reproducción del DAC simultáneamente.
 * Requiere que initialize_i2s_hardware() haya sido llamada previamente.
 */
void start_audio_streams(void);

/**
 * @brief Detener streams de audio
 * 
 * Detiene la captura y reproducción de audio. Útil para modo sleep
 * o cuando se necesita reconfigurar el hardware.
 */
void stop_audio_streams(void);

/**
 * @brief Reiniciar streams de audio
 * 
 * Combina stop_audio_streams() + delay + start_audio_streams()
 * para reinicio limpio del sistema de audio.
 */
void restart_audio_streams(void);

// ==================== FUNCIONES DE ESTADO ====================

/**
 * @brief Verificar si el hardware I2S está inicializado
 * 
 * @return true si initialize_i2s_hardware() se ejecutó exitosamente
 * @return false si el hardware no está configurado o hubo errores
 */
bool is_i2s_hardware_ready(void);

/**
 * @brief Verificar si los streams de audio están ejecutándose
 * 
 * @return true si los streams están activos y procesando audio
 * @return false si los streams están detenidos
 */
bool are_audio_streams_running(void);

// ==================== FUNCIONES DE INFORMACIÓN Y DIAGNÓSTICO ====================

/**
 * @brief Mostrar información de rendimiento del sistema I2S
 * 
 * Imprime por Serial:
 * - Estado de inicialización
 * - Parámetros de configuración
 * - Latencia teórica
 * - Uso de memoria
 * - Información de puertos I2S
 */
void get_audio_performance_info(void);

/**
 * @brief Ejecutar diagnóstico completo del hardware I2S
 * 
 * Realiza verificaciones exhaustivas del sistema:
 * - Configuración de pines
 * - Estado de drivers
 * - Información de memoria
 * - Recomendaciones de solución de problemas
 */
void diagnose_i2s_hardware(void);

// ==================== FUNCIONES DE CONFIGURACIÓN AVANZADA ====================

/**
 * @brief Obtener latencia actual del sistema de audio
 * 
 * Calcula la latencia teórica basada en el tamaño de buffer
 * y sample rate actual.
 * 
 * @return Latencia en milisegundos (float)
 */
float get_current_audio_latency_ms(void);

/**
 * @brief Obtener información detallada de memoria de audio
 * 
 * @param[out] total_allocated Memoria total asignada para I2S
 * @param[out] dma_buffers Memoria usada por buffers DMA
 * @param[out] driver_overhead Overhead del driver I2S
 */
void get_audio_memory_usage(size_t* total_allocated, size_t* dma_buffers, size_t* driver_overhead);

/**
 * @brief Verificar integridad de la configuración I2S
 * 
 * Realiza verificaciones de consistencia en la configuración
 * del hardware I2S para detectar posibles problemas.
 * 
 * @return ESP_OK si la configuración es válida
 * @return Código de error específico si hay problemas
 */
esp_err_t verify_i2s_configuration(void);

// ==================== FUNCIONES DE CONTROL DE POTENCIA ====================

/**
 * @brief Entrar en modo de bajo consumo para hardware I2S
 * 
 * Configura el hardware I2S para consumo mínimo de energía
 * manteniendo la capacidad de despertar rápidamente.
 * 
 * @note Los streams deben estar detenidos antes de llamar esta función
 */
void enter_i2s_low_power_mode(void);

/**
 * @brief Salir del modo de bajo consumo
 * 
 * Restaura la configuración normal del hardware I2S después
 * de salir del modo de bajo consumo.
 */
void exit_i2s_low_power_mode(void);

// ==================== FUNCIONES DE CALIBRACIÓN ====================

/**
 * @brief Ejecutar calibración automática del sistema de audio
 * 
 * Realiza ajustes automáticos para optimizar la calidad de audio:
 * - Verificación de niveles
 * - Ajuste de offset DC
 * - Optimización de timing
 * 
 * @return true si la calibración fue exitosa
 * @return false si se detectaron problemas
 */
bool run_audio_system_calibration(void);

/**
 * @brief Medir nivel de ruido de fondo del micrófono
 * 
 * Captura audio durante un período para medir el nivel
 * de ruido de fondo del micrófono en condiciones de silencio.
 * 
 * @param duration_ms Duración de la medición en milisegundos
 * @return Nivel de ruido RMS en dB SPL (aproximado)
 */
float measure_microphone_noise_floor(uint32_t duration_ms);

/**
 * @brief Verificar respuesta de frecuencia básica
 * 
 * Genera tonos de prueba internos y verifica que el sistema
 * de audio responda correctamente en diferentes frecuencias.
 * 
 * @param test_frequencies Array de frecuencias a probar (Hz)
 * @param num_frequencies Número de frecuencias en el array
 * @return true si todas las frecuencias pasan la prueba
 */
bool test_frequency_response(const float* test_frequencies, size_t num_frequencies);

// ==================== FUNCIONES DE DEBUGGING ====================

/**
 * @brief Habilitar/deshabilitar logging detallado de I2S
 * 
 * Controla el nivel de detalle en los mensajes de debug
 * del sistema I2S.
 * 
 * @param enable true para habilitar logging detallado
 */
void set_i2s_debug_logging(bool enable);

/**
 * @brief Mostrar estadísticas en tiempo real del I2S
 * 
 * Muestra información continua sobre el rendimiento del I2S:
 * - Buffers procesados
 * - Underruns/overruns
 * - Throughput actual
 * 
 * @param duration_seconds Duración del monitoreo
 */
void monitor_i2s_realtime_stats(uint32_t duration_seconds);

/**
 * @brief Generar reporte de configuración para soporte técnico
 * 
 * Genera un reporte completo de la configuración del sistema
 * para facilitar el soporte técnico y debugging.
 */
void generate_hardware_support_report(void);

// ==================== FUNCIONES EXPERIMENTALES ====================

/**
 * @brief Cambiar sample rate dinámicamente (EXPERIMENTAL)
 * 
 * Permite cambiar el sample rate sin reinicializar completamente
 * el hardware I2S.
 * 
 * @warning Esta función es experimental y puede causar glitches
 * @param new_sample_rate Nuevo sample rate en Hz
 * @return true si el cambio fue exitoso
 */
bool change_sample_rate_dynamic(uint32_t new_sample_rate);

/**
 * @brief Ajustar tamaño de buffer dinámicamente (EXPERIMENTAL)
 * 
 * Modifica el tamaño de buffer DMA para optimizar latencia vs estabilidad.
 * 
 * @warning Función experimental, puede afectar estabilidad
 * @param new_buffer_size Nuevo tamaño de buffer en muestras
 * @return true si el cambio fue exitoso
 */
bool adjust_buffer_size_dynamic(size_t new_buffer_size);

// ==================== MACROS DE CONVENIENCIA ====================

/**
 * @brief Macro para verificar si el hardware está listo antes de operaciones
 * 
 * Uso: ENSURE_HARDWARE_READY() antes de operaciones críticas
 */
#define ENSURE_HARDWARE_READY() \
    do { \
        if (!is_i2s_hardware_ready()) { \
            Serial.println("❌ Hardware I2S no está listo"); \
            return; \
        } \
    } while(0)

/**
 * @brief Macro para verificar si los streams están activos
 * 
 * Uso: ENSURE_STREAMS_RUNNING() antes de operaciones de audio
 */
#define ENSURE_STREAMS_RUNNING() \
    do { \
        if (!are_audio_streams_running()) { \
            Serial.println("❌ Streams de audio no están ejecutándose"); \
            return; \
        } \
    } while(0)

/**
 * @brief Macro para operaciones seguras con manejo de errores
 * 
 * @param operation Operación a ejecutar
 * @param error_msg Mensaje de error personalizado
 */
#define SAFE_I2S_OPERATION(operation, error_msg) \
    do { \
        esp_err_t _err = (operation); \
        if (_err != ESP_OK) { \
            Serial.printf("❌ %s: %s\n", error_msg, esp_err_to_name(_err)); \
            return _err; \
        } \
    } while(0)

// ==================== INFORMACIÓN DE HARDWARE ====================

/**
 * @brief Estructura con información detallada del hardware
 */
typedef struct {
    // Información del micrófono
    struct {
        const char* model;              // "ICS-43434"
        uint8_t bclk_pin;              // Pin BCLK
        uint8_t lrclk_pin;             // Pin LRCLK  
        uint8_t data_pin;              // Pin DATA
        uint32_t sample_rate;          // Sample rate actual
        i2s_bits_per_sample_t bits;    // Bits por muestra
        bool is_active;                // Estado activo
    } microphone;
    
    // Información del DAC
    struct {
        const char* model;              // "MAX98357A"
        uint8_t bclk_pin;              // Pin BCLK
        uint8_t lrclk_pin;             // Pin LRCLK
        uint8_t data_pin;              // Pin DATA
        uint32_t sample_rate;          // Sample rate actual
        i2s_bits_per_sample_t bits;    // Bits por muestra
        bool is_active;                // Estado activo
    } dac;
    
    // Información general
    struct {
        float latency_ms;              // Latencia calculada
        size_t buffer_size;            // Tamaño de buffer
        size_t memory_used;            // Memoria usada
        bool hardware_ready;           // Hardware inicializado
        bool streams_running;          // Streams activos
        uint32_t uptime_seconds;       // Tiempo de funcionamiento
    } system;
} hardware_info_t;

/**
 * @brief Obtener información completa del hardware
 * 
 * @param[out] info Estructura a llenar con información del hardware
 */
void get_hardware_info(hardware_info_t* info);

// ==================== COMENTARIOS DE IMPLEMENTACIÓN ====================

/*
 * NOTAS DE IMPLEMENTACIÓN:
 * 
 * 1. INICIALIZACIÓN:
 *    - initialize_i2s_hardware() debe llamarse UNA sola vez
 *    - start_audio_streams() puede llamarse múltiples veces
 *    - Verificar siempre el valor de retorno de las funciones
 * 
 * 2. MANEJO DE ERRORES:
 *    - Todas las funciones manejan errores internamente
 *    - Imprimen mensajes descriptivos por Serial
 *    - Funciones booleanas: true = éxito, false = error
 * 
 * 3. RENDIMIENTO:
 *    - Las funciones de diagnóstico son costosas, no usar en loops críticos
 *    - get_audio_performance_info() es segura para llamar frecuentemente
 *    - monitor_i2s_realtime_stats() bloquea durante la duración especificada
 * 
 * 4. MEMORIA:
 *    - El sistema I2S usa ~8KB de RAM para buffers DMA
 *    - Verificar siempre MIN_FREE_HEAP antes de inicializar
 *    - get_audio_memory_usage() proporciona detalles exactos
 * 
 * 5. CONCURRENCIA:
 *    - Todas las funciones son thread-safe
 *    - start/stop_audio_streams() pueden llamarse desde cualquier core
 *    - Las funciones de diagnóstico NO deben llamarse desde ISRs
 */

#endif // AUDIO_HARDWARE_H