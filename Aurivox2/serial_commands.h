// ==================== SERIAL_COMMANDS.H ====================
// Declaraciones para sistema completo de comandos seriales
// Interfaz de control completa con configuración persistente

#ifndef SERIAL_COMMANDS_H
#define SERIAL_COMMANDS_H

#include "audio_config.h"
#include "nvs.h"

// ==================== FUNCIONES PRINCIPALES ====================

/**
 * @brief Inicializar sistema completo de comandos seriales
 * 
 * Configura:
 * - Sistema NVS para configuración persistente
 * - Carga configuración default si existe
 * - Inicializa estructuras de datos internas
 * - Prepara interfaz de comandos
 * 
 * Debe llamarse una sola vez durante setup().
 */
void initialize_serial_interface(void);

/**
 * @brief Procesar comandos seriales pendientes
 * 
 * Lee y procesa comandos del puerto serial:
 * - Parsing de comandos y parámetros
 * - Ejecución de funciones correspondientes
 * - Respuestas formateadas por Serial
 * - Manejo de errores y validación
 * 
 * @note Debe llamarse frecuentemente desde Core 1 (tarea de control)
 * @note No bloquea si no hay datos disponibles
 */
void handle_serial_commands(void);

// ==================== FUNCIONES DE CONFIGURACIÓN PERSISTENTE ====================

/**
 * @brief Guardar configuración actual en NVS
 * 
 * Guarda la configuración completa del sistema:
 * - Ganancia actual
 * - Configuración de todos los algoritmos DSP
 * - Parámetros de botones y pips
 * - Configuraciones de conectividad
 * 
 * @param preset_name Nombre del preset (máx 31 caracteres)
 * @return true si se guardó correctamente
 * @return false si hubo error (NVS no disponible, nombre inválido, etc.)
 */
bool save_configuration_preset(const char* preset_name);

/**
 * @brief Cargar configuración desde NVS
 * 
 * Carga y aplica una configuración guardada:
 * - Verifica integridad con checksum
 * - Valida compatibilidad de versión
 * - Aplica configuración al sistema
 * - Sincroniza variables globales
 * 
 * @param preset_name Nombre del preset a cargar
 * @return true si se cargó y aplicó correctamente
 * @return false si no existe, está corrupto o hay error
 */
bool load_configuration_preset(const char* preset_name);

/**
 * @brief Eliminar preset de configuración
 * 
 * @param preset_name Nombre del preset a eliminar
 * @return true si se eliminó correctamente
 * @return false si no existe o hay error
 * 
 * @note No se puede eliminar el preset "default"
 */
bool delete_configuration_preset(const char* preset_name);

/**
 * @brief Listar todos los presets disponibles
 * 
 * Imprime por Serial todos los presets guardados en NVS.
 * Muestra también información de tamaño y fecha cuando esté disponible.
 */
void list_configuration_presets(void);

/**
 * @brief Restaurar configuración por defecto
 * 
 * Carga la configuración de fábrica sin afectar presets guardados.
 * Los cambios quedan en RAM hasta que se guarden explícitamente.
 */
void restore_default_configuration(void);

// ==================== FUNCIONES DE EXPORTACIÓN/IMPORTACIÓN ====================

/**
 * @brief Exportar configuración actual en formato texto
 * 
 * Genera salida por Serial en formato legible para software externo:
 * - Formato clave=valor
 * - Compatible con parsers externos
 * - Incluye todos los parámetros configurables
 * - Marcadores de inicio/fin para parsing automático
 */
void export_configuration_text(void);

/**
 * @brief Exportar configuración en formato JSON
 * 
 * Genera JSON estructurado con la configuración completa.
 * Ideal para aplicaciones web y APIs REST.
 */
void export_configuration_json(void);

/**
 * @brief Exportar configuración en formato binario
 * 
 * Genera dump hexadecimal de la estructura de configuración.
 * Útil para backup completo y transferencia entre dispositivos.
 */
void export_configuration_binary(void);

/**
 * @brief Importar configuración desde comando serial
 * 
 * Permite recibir configuración completa por comandos seriales.
 * Acepta formatos: texto (clave=valor), JSON y binario.
 * 
 * @param format Formato de los datos ("text", "json", "binary")
 * @param data Datos de configuración
 * @return true si se importó y aplicó correctamente
 */
bool import_configuration_data(const char* format, const char* data);

// ==================== FUNCIONES DE SISTEMA ====================

/**
 * @brief Mostrar ayuda completa del sistema de comandos
 * 
 * Imprime lista completa de comandos disponibles:
 * - Organizada por categorías
 * - Estado de implementación de cada comando
 * - Ejemplos de uso
 * - Parámetros requeridos
 */
void show_help_system(void);

/**
 * @brief Mostrar estado completo del sistema
 * 
 * Imprime información detallada:
 * - Hardware (I2S, botones, memoria)
 * - Audio (ganancia, algoritmos DSP)
 * - Configuración (presets, NVS)
 * - Conectividad (Bluetooth, WiFi)
 * - Rendimiento (CPU, latencia, memoria)
 */
void show_system_status(void);

/**
 * @brief Mostrar métricas de rendimiento
 * 
 * Información específica de rendimiento:
 * - Uso de CPU por core
 * - Memoria RAM disponible/usada
 * - Latencia de audio actual
 * - Throughput de I2S
 * - Estadísticas de tasks
 */
void show_performance_metrics(void);

/**
 * @brief Ejecutar diagnóstico completo del sistema
 * 
 * Diagnóstico exhaustivo de todos los subsistemas:
 * - Hardware I2S
 * - Sistema de botones
 * - NVS y configuración
 * - Memoria y rendimiento
 * - Recomendaciones de solución
 */
void run_system_diagnosis(void);

// ==================== FUNCIONES DE ALGORITMOS DSP ====================

/**
 * @brief Mostrar estado de todos los algoritmos DSP
 * 
 * Lista completa de algoritmos:
 * - Estado (implementado/no implementado)
 * - Configuración actual
 * - Parámetros disponibles
 * - Comandos relacionados
 */
void show_dsp_algorithms_status(void);

/**
 * @brief Aplicar preset de algoritmos DSP
 * 
 * Presets predefinidos para casos de uso comunes:
 * 
 * @param preset_type Tipo de preset a aplicar
 * @return true si el preset es válido y se aplicó
 */
bool apply_dsp_preset(PresetType preset_type);

/**
 * @brief Obtener información detallada de algoritmo específico
 * 
 * @param algorithm_name Nombre del algoritmo ("eq", "wdrc", "limiter", etc.)
 */
void show_algorithm_details(const char* algorithm_name);

// ==================== FUNCIONES DE DATOS MÉDICOS ====================

/**
 * @brief Mostrar audiograma completo del paciente
 * 
 * Muestra tabla de umbrales audiométricos:
 * - Frecuencias estándar (125Hz - 8kHz)
 * - Ambos oídos por separado
 * - Formato tabular legible
 * - Gráfico ASCII si es posible
 */
void show_audiometry_data(void);

/**
 * @brief Exportar datos médicos completos
 * 
 * Genera reporte médico formateado:
 * - Información del paciente
 * - Audiograma completo
 * - Configuración de audífono
 * - Historial de sesiones
 * - Notas clínicas
 * - Formato compatible con sistemas médicos
 */
void export_medical_report(void);

/**
 * @brief Establecer información del paciente
 * 
 * @param field Campo a establecer ("name", "age", "id", etc.)
 * @param value Valor a asignar
 * @return true si el campo es válido y se estableció
 */
bool set_patient_information(const char* field, const char* value);

/**
 * @brief Agregar nota clínica al historial
 * 
 * @param note Nota clínica (máximo 512 caracteres)
 * @param timestamp Timestamp de la nota (0 = usar actual)
 * @return true si se agregó correctamente
 */
bool add_clinical_note(const char* note, uint32_t timestamp);

// ==================== FUNCIONES DE CONECTIVIDAD ====================

/**
 * @brief Mostrar estado de conectividad
 * 
 * Información de todas las conexiones:
 * - Bluetooth A2DP
 * - WiFi y control remoto
 * - CROSS/BiCROSS
 * - Dispositivos pareados
 */
void show_connectivity_status(void);

/**
 * @brief Configurar modo de conectividad
 * 
 * @param mode Modo a configurar ("standalone", "cross", "bicross", "bluetooth")
 * @return true si el modo es válido y se aplicó
 */
bool set_connectivity_mode(const char* mode);

// ==================== FUNCIONES DE VALIDACIÓN ====================

/**
 * @brief Validar comando y parámetros
 * 
 * Verifica sintaxis y validez de comandos antes de ejecutar.
 * 
 * @param command Comando a validar
 * @param parameters Array de parámetros
 * @param param_count Número de parámetros
 * @return true si el comando y parámetros son válidos
 */
bool validate_command_syntax(const char* command, const char** parameters, int param_count);

/**
 * @brief Verificar permisos para comando
 * 
 * Algunos comandos pueden requerir confirmación o tener restricciones.
 * 
 * @param command Comando a verificar
 * @return true si se puede ejecutar el comando
 */
bool check_command_permissions(const char* command);

// ==================== FUNCIONES DE LOGGING ====================

/**
 * @brief Establecer nivel de logging
 * 
 * @param level Nivel de detalle (0=mínimo, 3=máximo debug)
 */
void set_logging_level(int level);

/**
 * @brief Habilitar/deshabilitar logging de comandos
 * 
 * @param enabled true para registrar todos los comandos ejecutados
 */
void set_command_logging(bool enabled);

/**
 * @brief Mostrar historial de comandos recientes
 * 
 * @param count Número de comandos recientes a mostrar (máx 50)
 */
void show_command_history(int count);

// ==================== TIPOS DE DATOS Y ESTRUCTURAS ====================

/**
 * @brief Información de comando individual
 */
typedef struct {
    const char* name;           // Nombre del comando
    const char* description;    // Descripción breve
    const char* usage;          // Ejemplo de uso
    const char* category;       // Categoría del comando
    bool implemented;           // Si está implementado
    int min_params;            // Parámetros mínimos requeridos
    int max_params;            // Parámetros máximos aceptados
} command_info_t;

/**
 * @brief Estructura de respuesta de comando
 */
typedef struct {
    bool success;              // Si el comando se ejecutó exitosamente
    const char* message;       // Mensaje de respuesta
    const char* error_detail;  // Detalle del error (si aplica)
    int error_code;           // Código de error específico
} command_response_t;

/**
 * @brief Estadísticas del sistema de comandos
 */
typedef struct {
    uint32_t total_commands_executed;    // Total de comandos ejecutados
    uint32_t successful_commands;        // Comandos exitosos
    uint32_t failed_commands;           // Comandos fallidos
    uint32_t invalid_commands;          // Comandos inválidos
    uint32_t uptime_seconds;            // Tiempo activo del sistema
    uint32_t average_response_time_us;   // Tiempo promedio de respuesta
} command_system_stats_t;

// ==================== FUNCIONES DE INFORMACIÓN ====================

/**
 * @brief Obtener información de comando específico
 * 
 * @param command_name Nombre del comando
 * @param[out] info Estructura a llenar con información
 * @return true si el comando existe
 */
bool get_command_info(const char* command_name, command_info_t* info);

/**
 * @brief Obtener estadísticas del sistema de comandos
 * 
 * @param[out] stats Estructura a llenar con estadísticas
 */
void get_command_system_stats(command_system_stats_t* stats);

/**
 * @brief Obtener lista de todos los comandos disponibles
 * 
 * @param[out] commands Array a llenar con nombres de comandos
 * @param max_commands Tamaño máximo del array
 * @return Número de comandos disponibles
 */
int get_available_commands(const char** commands, int max_commands);

// ==================== FUNCIONES DE CALLBACK ====================

/**
 * @brief Tipo de función callback para comandos personalizados
 * 
 * @param command Comando ejecutado
 * @param params Array de parámetros
 * @param param_count Número de parámetros
 * @return Respuesta del comando
 */
typedef command_response_t (*custom_command_callback_t)(const char* command, 
                                                       const char** params, 
                                                       int param_count);

/**
 * @brief Registrar comando personalizado
 * 
 * Permite agregar comandos customizados al sistema.
 * 
 * @param command_name Nombre del comando (único)
 * @param callback Función a ejecutar
 * @param description Descripción del comando
 * @return true si se registró correctamente
 */
bool register_custom_command(const char* command_name, 
                            custom_command_callback_t callback,
                            const char* description);

/**
 * @brief Desregistrar comando personalizado
 * 
 * @param command_name Nombre del comando a eliminar
 * @return true si se eliminó correctamente
 */
bool unregister_custom_command(const char* command_name);

// ==================== CONSTANTES ====================

// Códigos de error
#define CMD_ERROR_NONE                 0
#define CMD_ERROR_UNKNOWN_COMMAND      1
#define CMD_ERROR_INVALID_PARAMS       2
#define CMD_ERROR_NOT_IMPLEMENTED      3
#define CMD_ERROR_PERMISSION_DENIED    4
#define CMD_ERROR_SYSTEM_ERROR         5
#define CMD_ERROR_HARDWARE_NOT_READY   6
#define CMD_ERROR_INVALID_STATE        7

// Límites del sistema
#define MAX_COMMAND_LENGTH            128
#define MAX_PARAMETERS               10
#define MAX_PARAMETER_LENGTH         64
#define MAX_CUSTOM_COMMANDS          20
#define COMMAND_HISTORY_SIZE         50

// Categorías de comandos
#define CMD_CATEGORY_SYSTEM          "Sistema"
#define CMD_CATEGORY_AUDIO           "Audio"
#define CMD_CATEGORY_DSP             "DSP"
#define CMD_CATEGORY_CONFIG          "Configuración"
#define CMD_CATEGORY_MEDICAL         "Médico"
#define CMD_CATEGORY_CONNECTIVITY    "Conectividad"
#define CMD_CATEGORY_DEBUG           "Debug"

// ==================== MACROS DE CONVENIENCIA ====================

/**
 * @brief Macro para respuesta exitosa
 */
#define CMD_SUCCESS(msg) \
    { .success = true, .message = (msg), .error_detail = NULL, .error_code = CMD_ERROR_NONE }

/**
 * @brief Macro para respuesta de error
 */
#define CMD_ERROR(msg, detail, code) \
    { .success = false, .message = (msg), .error_detail = (detail), .error_code = (code) }

/**
 * @brief Macro para comando no implementado
 */
#define CMD_NOT_IMPLEMENTED(cmd) \
    CMD_ERROR("Comando no implementado", "Este comando está planificado para futuras versiones", CMD_ERROR_NOT_IMPLEMENTED)

// ==================== COMENTARIOS DE IMPLEMENTACIÓN ====================

/*
 * NOTAS DE IMPLEMENTACIÓN:
 * 
 * 1. PARSING DE COMANDOS:
 *    - Convierte automáticamente a lowercase
 *    - Separa comando y parámetros por espacios
 *    - Valida número de parámetros antes de ejecutar
 *    - Maneja comillas para parámetros con espacios
 * 
 * 2. CONFIGURACIÓN PERSISTENTE:
 *    - Usa NVS para almacenamiento no volátil
 *    - Checksum automático para integridad
 *    - Versionado para compatibilidad futura
 *    - Backup/restore de configuraciones
 * 
 * 3. RESPUESTAS FORMATEADAS:
 *    - Prefijos emoji para identificar tipo de mensaje
 *    - Formato consistente para todas las respuestas
 *    - Separadores visuales para mejor legibilidad
 *    - Códigos de error estándar
 * 
 * 4. EXTENSIBILIDAD:
 *    - Sistema de callbacks para comandos personalizados
 *    - Validación automática de sintaxis
 *    - Logging opcional de actividad
 *    - Estadísticas de uso
 * 
 * 5. CONCURRENCIA:
 *    - handle_serial_commands() desde Core 1 únicamente
 *    - initialize_serial_interface() solo desde setup()
 *    - Funciones de configuración son thread-safe
 *    - NVS operations tienen mutex interno
 */

#endif // SERIAL_COMMANDS_H