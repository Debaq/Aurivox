// ==================== BUTTON_CONTROL.H ====================
// Declaraciones para sistema de control de botones y feedback de audio
// Manejo de ISRs, debounce, sleep mode y sistema de pips

#ifndef BUTTON_CONTROL_H
#define BUTTON_CONTROL_H

#include "audio_config.h"

// ==================== FUNCIONES PRINCIPALES DE BOTONES ====================

/**
 * @brief Inicializar sistema completo de botones
 * 
 * Configura:
 * - Pines de botones como INPUT_PULLUP
 * - ISRs con debounce automático
 * - Wake-up para sleep mode
 * - Sistema de pips interno
 * 
 * Debe llamarse una sola vez durante setup().
 */
void initialize_buttons(void);

/**
 * @brief Manejar eventos de botones pendientes
 * 
 * Procesa flags establecidos por las ISRs:
 * - Botones de ganancia (subir/bajar)
 * - Botón de sleep (hold detection)
 * - Lógica de debounce por software
 * 
 * @note Debe llamarse frecuentemente desde Core 1 (tarea de control)
 * @note No bloquea, solo procesa eventos pendientes
 */
void handle_button_events(void);

// ==================== FUNCIONES DEL SISTEMA DE PIPS ====================

/**
 * @brief Procesar audio de pips para el pipeline DSP
 * 
 * Genera tonos de feedback para confirmación de acciones:
 * - 1-5 pips según nivel de ganancia
 * - 3 pips para límites (máximo/mínimo)
 * - Secuencias especiales para sleep/wake
 * 
 * @param[out] buffer Buffer de audio a llenar con pips
 * @param num_samples Número de muestras a procesar
 * @return true si se generaron pips (buffer modificado)
 * @return false si no hay pips activos (buffer sin modificar)
 * 
 * @note Debe llamarse desde Core 0 (tarea de audio)
 * @note Se integra en el pipeline DSP principal
 */
bool process_pip_audio(int16_t* buffer, int num_samples);

/**
 * @brief Verificar si hay pips activos
 * 
 * @return true si el sistema de pips está generando audio
 * @return false si no hay pips en reproducción
 */
bool are_pips_active(void);

/**
 * @brief Forzar detención de pips
 * 
 * Detiene inmediatamente cualquier secuencia de pips en progreso.
 * Útil para comandos seriales o situaciones de emergencia.
 */
void force_stop_pips(void);

// ==================== FUNCIONES DE CONTROL DE GANANCIA ====================

/**
 * @brief Incrementar nivel de ganancia
 * 
 * Sube un nivel la ganancia (máximo 5 niveles).
 * Genera pips de confirmación automáticamente.
 * 
 * @return true si se incrementó la ganancia
 * @return false si ya estaba al máximo
 */
bool increment_gain_level(void);

/**
 * @brief Decrementar nivel de ganancia
 * 
 * Baja un nivel la ganancia (mínimo nivel 1).
 * Genera pips de confirmación automáticamente.
 * 
 * @return true si se decrementó la ganancia
 * @return false si ya estaba al mínimo
 */
bool decrement_gain_level(void);

/**
 * @brief Establecer nivel de ganancia específico
 * 
 * Cambia directamente a un nivel específico sin pips automáticos.
 * 
 * @param level Nivel de ganancia (1-5)
 * @return true si el nivel es válido y se aplicó
 * @return false si el nivel está fuera de rango
 */
bool set_gain_level(int level);

/**
 * @brief Obtener nivel de ganancia actual
 * 
 * @return Nivel actual (1-5)
 */
int get_current_gain_level(void);

/**
 * @brief Obtener factor de ganancia lineal actual
 * 
 * @return Factor de ganancia (0.0 - 1.0)
 */
float get_current_gain_factor(void);

// ==================== FUNCIONES DE SLEEP MODE ====================

/**
 * @brief Entrar manualmente en sleep mode
 * 
 * Fuerza la entrada en sleep sin necesidad de presionar el botón.
 * Útil para comandos seriales o control remoto.
 */
void enter_sleep_mode_manual(void);

/**
 * @brief Verificar si el sistema está en sleep
 * 
 * @return true si el sistema está durmiendo
 * @return false si el sistema está activo
 */
bool is_system_sleeping(void);

/**
 * @brief Configurar tiempo de hold para sleep
 * 
 * Cambia el tiempo requerido para activar sleep mode.
 * 
 * @param hold_time_ms Tiempo en milisegundos (mínimo 1000ms)
 * @return true si el tiempo es válido y se aplicó
 */
bool set_sleep_hold_time(uint32_t hold_time_ms);

/**
 * @brief Obtener tiempo de hold configurado
 * 
 * @return Tiempo actual de hold en milisegundos
 */
uint32_t get_sleep_hold_time(void);

// ==================== FUNCIONES DE CONFIGURACIÓN DE PIPS ====================

/**
 * @brief Configurar parámetros del sistema de pips
 * 
 * @param frequency Frecuencia del tono en Hz (100-5000)
 * @param amplitude Amplitud del tono (0.0-1.0)
 * @param duration_ms Duración de cada pip en ms (50-1000)
 * @param gap_ms Silencio entre pips en ms (10-500)
 * @return true si todos los parámetros son válidos
 */
bool configure_pip_system(float frequency, float amplitude, 
                         uint32_t duration_ms, uint32_t gap_ms);

/**
 * @brief Obtener configuración actual de pips
 * 
 * @param[out] frequency Frecuencia actual en Hz
 * @param[out] amplitude Amplitud actual (0.0-1.0)
 * @param[out] duration_ms Duración actual en ms
 * @param[out] gap_ms Gap actual en ms
 */
void get_pip_configuration(float* frequency, float* amplitude,
                          uint32_t* duration_ms, uint32_t* gap_ms);

/**
 * @brief Reproducir secuencia de pips personalizada
 * 
 * Inicia una secuencia de pips con configuración específica.
 * 
 * @param num_pips Número de pips a reproducir (1-10)
 * @param frequency Frecuencia específica en Hz (opcional, 0 = usar default)
 * @param amplitude Amplitud específica (opcional, 0 = usar default)
 * @return true si la secuencia se inició correctamente
 */
bool play_custom_pip_sequence(int num_pips, float frequency, float amplitude);

// ==================== FUNCIONES DE DIAGNÓSTICO ====================

/**
 * @brief Mostrar estado completo del sistema de botones
 * 
 * Imprime por Serial:
 * - Estado actual de cada botón (HIGH/LOW)
 * - Nivel de ganancia actual
 * - Estado de pips
 * - Estado de sleep
 * - Configuración de timing
 */
void get_button_status(void);

/**
 * @brief Ejecutar test completo del sistema de botones
 * 
 * Realiza pruebas automáticas:
 * - Test de pips (1-5 secuencias)
 * - Instrucciones para test manual de botones
 * - Verificación de respuesta
 */
void test_button_system(void);

/**
 * @brief Verificar integridad de las ISRs
 * 
 * Comprueba que las ISRs estén configuradas correctamente
 * y respondan a interrupciones.
 * 
 * @return true si todas las ISRs funcionan correctamente
 */
bool verify_isr_integrity(void);

/**
 * @brief Medir tiempo de respuesta de botones
 * 
 * Mide el tiempo entre presión de botón y procesamiento.
 * Útil para verificar el rendimiento del debounce.
 * 
 * @param button_pin Pin del botón a medir
 * @param num_samples Número de mediciones a promediar
 * @return Tiempo promedio de respuesta en microsegundos
 */
uint32_t measure_button_response_time(uint8_t button_pin, int num_samples);

// ==================== FUNCIONES DE CONFIGURACIÓN AVANZADA ====================

/**
 * @brief Configurar tiempo de debounce
 * 
 * Ajusta el tiempo de debounce para todos los botones.
 * 
 * @param debounce_ms Tiempo de debounce en milisegundos (10-200)
 * @return true si el tiempo es válido y se aplicó
 */
bool set_debounce_time(uint32_t debounce_ms);

/**
 * @brief Obtener tiempo de debounce actual
 * 
 * @return Tiempo de debounce en milisegundos
 */
uint32_t get_debounce_time(void);

/**
 * @brief Habilitar/deshabilitar botón específico
 * 
 * Permite deshabilitar botones individualmente.
 * 
 * @param button_pin Pin del botón (BTN_GAIN_UP, BTN_GAIN_DOWN, BTN_SLEEP)
 * @param enabled true para habilitar, false para deshabilitar
 * @return true si el pin es válido y se aplicó el cambio
 */
bool set_button_enabled(uint8_t button_pin, bool enabled);

/**
 * @brief Verificar si un botón está habilitado
 * 
 * @param button_pin Pin del botón a verificar
 * @return true si el botón está habilitado
 */
bool is_button_enabled(uint8_t button_pin);

// ==================== FUNCIONES DE CALLBACK ====================

/**
 * @brief Tipo de función callback para eventos de botones
 * 
 * @param button_pin Pin del botón que generó el evento
 * @param event_type Tipo de evento (press, release, hold)
 * @param hold_duration Para eventos hold, duración en ms
 */
typedef void (*button_callback_t)(uint8_t button_pin, int event_type, uint32_t hold_duration);

/**
 * @brief Registrar callback para eventos de botones
 * 
 * Permite registrar una función que se llamará cuando ocurran eventos de botones.
 * 
 * @param callback Función a llamar, NULL para deshabilitar
 */
void register_button_callback(button_callback_t callback);

// ==================== CONSTANTES Y ENUMERACIONES ====================

// Tipos de eventos de botones
enum ButtonEvent {
    BUTTON_EVENT_PRESS = 0,     // Botón presionado
    BUTTON_EVENT_RELEASE = 1,   // Botón liberado
    BUTTON_EVENT_HOLD = 2,      // Botón mantenido
    BUTTON_EVENT_DOUBLE = 3     // Doble click (futuro)
};

// Tipos de secuencias de pips predefinidas
enum PipSequence {
    PIP_SEQUENCE_GAIN_LEVEL = 0,    // Pips según nivel de ganancia
    PIP_SEQUENCE_LIMIT_REACHED = 1, // 3 pips para límites
    PIP_SEQUENCE_SLEEP_WARNING = 2, // 3 pips antes de sleep
    PIP_SEQUENCE_WAKE_CONFIRM = 3,  // 2 pips al despertar
    PIP_SEQUENCE_ERROR = 4,         // 1 pip largo para errores
    PIP_SEQUENCE_SUCCESS = 5        // 2 pips rápidos para éxito
};

// Estados del sistema de botones
enum ButtonSystemState {
    BUTTON_SYSTEM_INITIALIZING = 0,
    BUTTON_SYSTEM_ACTIVE = 1,
    BUTTON_SYSTEM_SLEEPING = 2,
    BUTTON_SYSTEM_ERROR = 3
};

// ==================== MACROS DE CONVENIENCIA ====================

/**
 * @brief Macro para verificar si el sistema de botones está activo
 */
#define IS_BUTTON_SYSTEM_ACTIVE() (!is_system_sleeping())

/**
 * @brief Macro para pips de confirmación rápida
 */
#define PLAY_CONFIRMATION_PIPS() play_custom_pip_sequence(2, 0, 0)

/**
 * @brief Macro para pips de error
 */
#define PLAY_ERROR_PIPS() play_custom_pip_sequence(1, 500, 0.8)

/**
 * @brief Macro para verificar rango de ganancia
 */
#define IS_VALID_GAIN_LEVEL(level) ((level) >= 1 && (level) <= GAIN_LEVELS_COUNT)

// ==================== ESTRUCTURA DE INFORMACIÓN ====================

/**
 * @brief Información completa del sistema de botones
 */
typedef struct {
    // Estado de botones individuales
    struct {
        bool gain_up_pressed;       // Estado actual del botón
        bool gain_down_pressed;     // Estado actual del botón
        bool sleep_pressed;         // Estado actual del botón
        bool gain_up_enabled;       // Botón habilitado/deshabilitado
        bool gain_down_enabled;     // Botón habilitado/deshabilitado
        bool sleep_enabled;         // Botón habilitado/deshabilitado
    } buttons;
    
    // Configuración de timing
    struct {
        uint32_t debounce_ms;       // Tiempo de debounce
        uint32_t sleep_hold_ms;     // Tiempo de hold para sleep
        uint32_t last_interrupt;    // Último tiempo de interrupción
    } timing;
    
    // Estado de ganancia
    struct {
        int current_level;          // Nivel actual (1-5)
        float current_factor;       // Factor lineal actual
        bool auto_pips_enabled;     // Pips automáticos habilitados
    } gain;
    
    // Estado de pips
    struct {
        bool active;                // Pips activos
        int remaining;              // Pips restantes
        float frequency;            // Frecuencia actual
        float amplitude;            // Amplitud actual
        uint32_t duration_ms;       // Duración por pip
        uint32_t gap_ms;           // Gap entre pips
    } pips;
    
    // Estado del sistema
    struct {
        ButtonSystemState state;    // Estado general
        bool sleeping;              // Sistema durmiendo
        uint32_t uptime_seconds;    // Tiempo de funcionamiento
        uint32_t total_button_presses; // Total de presiones
    } system;
} button_system_info_t;

/**
 * @brief Obtener información completa del sistema de botones
 * 
 * @param[out] info Estructura a llenar con información del sistema
 */
void get_button_system_info(button_system_info_t* info);

// ==================== FUNCIONES DE PERSISTENCIA ====================

/**
 * @brief Guardar configuración de botones en NVS
 * 
 * Guarda configuración actual de:
 * - Tiempos de debounce y hold
 * - Configuración de pips
 * - Estados habilitados/deshabilitados
 * 
 * @param preset_name Nombre del preset a guardar
 * @return true si se guardó correctamente
 */
bool save_button_config(const char* preset_name);

/**
 * @brief Cargar configuración de botones desde NVS
 * 
 * @param preset_name Nombre del preset a cargar
 * @return true si se cargó correctamente
 */
bool load_button_config(const char* preset_name);

// ==================== COMENTARIOS DE IMPLEMENTACIÓN ====================

/*
 * NOTAS DE IMPLEMENTACIÓN:
 * 
 * 1. INTERRUPCIONES:
 *    - ISRs implementadas con debounce en hardware
 *    - Flags volátiles para comunicación ISR → main loop
 *    - handle_button_events() debe llamarse frecuentemente
 * 
 * 2. SISTEMA DE PIPS:
 *    - Integrado en pipeline DSP principal
 *    - No usar delay() en funciones de pips
 *    - process_pip_audio() es thread-safe para Core 0
 * 
 * 3. SLEEP MODE:
 *    - Wake-up configurado automáticamente en initialize_buttons()
 *    - Sleep detiene processing pero mantiene ISRs activas
 *    - enter_sleep_mode_manual() disponible para control remoto
 * 
 * 4. GANANCIA:
 *    - Niveles 1-5 corresponden a índices 0-4 internamente
 *    - Cambios de ganancia generan pips automáticos por defecto
 *    - set_gain_level() NO genera pips automáticos
 * 
 * 5. CONCURRENCIA:
 *    - initialize_buttons() solo desde setup()
 *    - handle_button_events() desde Core 1 únicamente
 *    - process_pip_audio() desde Core 0 únicamente
 *    - Todas las demás funciones son thread-safe
 */

#endif // BUTTON_CONTROL_H