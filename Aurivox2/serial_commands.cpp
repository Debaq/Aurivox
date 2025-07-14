// ==================== SERIAL_COMMANDS.CPP ====================
// Sistema completo de comandos seriales para Aurivox v3.0
// Todos los comandos implementados desde el inicio, funcionalidades que no existen
// responden apropiadamente indicando su estado "no implementado"

#include "Arduino.h"
#include "nvs_flash.h"
#include "nvs.h"

// ==================== VARIABLES EXTERNAS ====================

// Variables del sistema principal
extern volatile bool audio_processing_active;
extern volatile bool system_sleeping;
extern volatile int current_gain_level;
extern volatile float gain_factor;
extern const float gain_levels[5];

// Funciones externas de otros módulos
extern void get_audio_performance_info();
extern void diagnose_i2s_hardware();
extern bool is_i2s_hardware_ready();
extern bool are_audio_streams_running();
extern void get_button_status();
extern void test_button_system();
extern bool are_pips_active();
extern void force_stop_pips();

// ==================== CONFIGURACIÓN NVS ====================

static nvs_handle_t nvs_config_handle;
static bool nvs_initialized = false;

// ==================== ESTRUCTURAS DE CONFIGURACIÓN ====================

// Estructura base de configuración (será expandida)
struct AudioConfig {
  uint32_t version;
  int gain_level;
  
  // Filtro pasa-altos (no implementado aún)
  bool highpass_enabled;
  float highpass_freq;
  
  // Ecualizador (no implementado aún)
  bool eq_enabled;
  float eq_gains[6];  // 250Hz, 500Hz, 1kHz, 2kHz, 4kHz, 8kHz
  
  // WDRC (no implementado aún)
  bool wdrc_enabled;
  float wdrc_threshold;
  float wdrc_ratio;
  float wdrc_attack;
  float wdrc_release;
  
  // Limitador (no implementado aún)
  bool limiter_enabled;
  float limiter_threshold;
  
  uint32_t checksum;
};

// Configuración actual en RAM
static AudioConfig current_config;

// Configuración por defecto
static const AudioConfig default_config = {
  .version = 1,
  .gain_level = 2,  // 50%
  .highpass_enabled = false,
  .highpass_freq = 100.0,
  .eq_enabled = false,
  .eq_gains = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
  .wdrc_enabled = false,
  .wdrc_threshold = -20.0,
  .wdrc_ratio = 2.0,
  .wdrc_attack = 10.0,
  .wdrc_release = 100.0,
  .limiter_enabled = false,
  .limiter_threshold = -6.0,
  .checksum = 0
};

// ==================== FUNCIONES DE CONFIGURACIÓN ====================

static uint32_t calculate_checksum(const AudioConfig* config) {
  uint32_t checksum = 0;
  const uint8_t* data = (const uint8_t*)config;
  size_t size = sizeof(AudioConfig) - sizeof(uint32_t);
  
  for (size_t i = 0; i < size; i++) {
    checksum += data[i];
  }
  return checksum;
}

static void sync_config_to_system() {
  current_gain_level = current_config.gain_level;
  gain_factor = gain_levels[current_gain_level];
  // TODO: Sincronizar otros parámetros cuando se implementen
}

static void sync_system_to_config() {
  current_config.version = 1;
  current_config.gain_level = current_gain_level;
  // TODO: Sincronizar otros parámetros cuando se implementen
  current_config.checksum = calculate_checksum(&current_config);
}

static bool load_config_from_nvs(const char* preset_name) {
  if (!nvs_initialized) {
    Serial.println("❌ NVS no inicializado");
    return false;
  }
  
  size_t required_size = sizeof(AudioConfig);
  AudioConfig loaded_config;
  
  esp_err_t err = nvs_get_blob(nvs_config_handle, preset_name, &loaded_config, &required_size);
  
  if (err == ESP_OK) {
    uint32_t calculated_checksum = calculate_checksum(&loaded_config);
    if (calculated_checksum == loaded_config.checksum && loaded_config.version == 1) {
      current_config = loaded_config;
      sync_config_to_system();
      Serial.printf("✅ Configuración '%s' cargada desde memoria\n", preset_name);
      return true;
    } else {
      Serial.printf("❌ Configuración '%s' corrupta (checksum inválido)\n", preset_name);
    }
  } else if (err == ESP_ERR_NVS_NOT_FOUND) {
    Serial.printf("⚠️ Configuración '%s' no encontrada\n", preset_name);
  } else {
    Serial.printf("❌ Error cargando configuración '%s': %s\n", preset_name, esp_err_to_name(err));
  }
  
  return false;
}

static bool save_config_to_nvs(const char* preset_name) {
  if (!nvs_initialized) {
    Serial.println("❌ NVS no inicializado");
    return false;
  }
  
  sync_system_to_config();
  
  esp_err_t err = nvs_set_blob(nvs_config_handle, preset_name, &current_config, sizeof(AudioConfig));
  
  if (err == ESP_OK) {
    err = nvs_commit(nvs_config_handle);
    if (err == ESP_OK) {
      Serial.printf("✅ Configuración guardada como '%s'\n", preset_name);
      return true;
    }
  }
  
  Serial.printf("❌ Error guardando configuración '%s': %s\n", preset_name, esp_err_to_name(err));
  return false;
}

// ==================== COMANDOS DE AYUDA ====================

static void show_help() {
  Serial.println("\n📋 AURIVOX v3.0 - COMANDOS DISPONIBLES");
  Serial.println("════════════════════════════════════════════════════════════");
  
  Serial.println("🔧 SISTEMA:");
  Serial.println("  help / h                    → Esta ayuda");
  Serial.println("  status / info               → Estado completo del sistema");
  Serial.println("  reset                       → Restaurar configuración default");
  Serial.println("  performance                 → Métricas de rendimiento");
  Serial.println("  diagnose                    → Diagnóstico completo");
  Serial.println("  test_buttons                → Test del sistema de botones");
  Serial.println("");
  
  Serial.println("🔊 GANANCIA:");
  Serial.println("  set_gain_level <1-5>        → Cambiar ganancia (✅ IMPLEMENTADO)");
  Serial.println("  get_gain_level              → Ver ganancia actual (✅ IMPLEMENTADO)");
  Serial.println("");
  
  Serial.println("🎛️ FILTRO PASA-ALTOS:");
  Serial.println("  enable_highpass             → Activar filtro (🚧 NO IMPLEMENTADO)");
  Serial.println("  disable_highpass            → Desactivar filtro (🚧 NO IMPLEMENTADO)");
  Serial.println("  set_highpass_freq <Hz>      → Cambiar frecuencia (🚧 NO IMPLEMENTADO)");
  Serial.println("  get_highpass_status         → Ver estado del filtro (🚧 NO IMPLEMENTADO)");
  Serial.println("");
  
  Serial.println("🎵 ECUALIZADOR 6 BANDAS:");
  Serial.println("  enable_equalizer            → Activar ecualizador (🚧 NO IMPLEMENTADO)");
  Serial.println("  disable_equalizer           → Desactivar ecualizador (🚧 NO IMPLEMENTADO)");
  Serial.println("  set_eq_band <1-6> <dB>      → Ajustar banda -20 a +20dB (🚧 NO IMPLEMENTADO)");
  Serial.println("  get_eq_bands                → Ver todas las bandas (🚧 NO IMPLEMENTADO)");
  Serial.println("  reset_eq                    → Todas las bandas a 0dB (🚧 NO IMPLEMENTADO)");
  Serial.println("  eq_preset_flat              → Preset plano (🚧 NO IMPLEMENTADO)");
  Serial.println("  eq_preset_speech            → Preset para voz (🚧 NO IMPLEMENTADO)");
  Serial.println("");
  
  Serial.println("🎚️ WDRC (COMPRESIÓN):");
  Serial.println("  enable_wdrc                 → Activar compresión (🚧 NO IMPLEMENTADO)");
  Serial.println("  disable_wdrc                → Desactivar compresión (🚧 NO IMPLEMENTADO)");
  Serial.println("  set_wdrc_threshold <dB>     → Umbral de compresión (🚧 NO IMPLEMENTADO)");
  Serial.println("  set_wdrc_ratio <ratio>      → Ratio de compresión (🚧 NO IMPLEMENTADO)");
  Serial.println("  set_wdrc_attack <ms>        → Tiempo de attack (🚧 NO IMPLEMENTADO)");
  Serial.println("  set_wdrc_release <ms>       → Tiempo de release (🚧 NO IMPLEMENTADO)");
  Serial.println("  get_wdrc_status             → Ver configuración WDRC (🚧 NO IMPLEMENTADO)");
  Serial.println("");
  
  Serial.println("🛡️ LIMITADOR:");
  Serial.println("  enable_limiter              → Activar limitador (🚧 NO IMPLEMENTADO)");
  Serial.println("  disable_limiter             → Desactivar limitador (🚧 NO IMPLEMENTADO)");
  Serial.println("  set_limiter_threshold <dB>  → Umbral del limitador (🚧 NO IMPLEMENTADO)");
  Serial.println("  get_limiter_status          → Ver estado del limitador (🚧 NO IMPLEMENTADO)");
  Serial.println("");
  
  Serial.println("💾 CONFIGURACIÓN PERSISTENTE:");
  Serial.println("  save_preset <nombre>        → Guardar configuración actual (✅ IMPLEMENTADO)");
  Serial.println("  load_preset <nombre>        → Cargar configuración (✅ IMPLEMENTADO)");
  Serial.println("  list_presets                → Ver configuraciones guardadas (✅ IMPLEMENTADO)");
  Serial.println("  delete_preset <nombre>      → Eliminar configuración (✅ IMPLEMENTADO)");
  Serial.println("  export_config               → Exportar config para software (✅ IMPLEMENTADO)");
  Serial.println("");
  
  Serial.println("🏥 DATOS MÉDICOS:");
  Serial.println("  set_audiometry <freq> <db>  → Ingresar umbral audiométrico (🚧 NO IMPLEMENTADO)");
  Serial.println("  show_audiometry             → Ver audiograma completo (🚧 NO IMPLEMENTADO)");
  Serial.println("  set_patient_info <data>     → Datos del paciente (🚧 NO IMPLEMENTADO)");
  Serial.println("  export_medical_data         → Exportar datos médicos (🚧 NO IMPLEMENTADO)");
  Serial.println("");
  
  Serial.println("📱 CONECTIVIDAD:");
  Serial.println("  enable_bluetooth            → Activar Bluetooth A2DP (🚧 NO IMPLEMENTADO)");
  Serial.println("  disable_bluetooth           → Desactivar Bluetooth (🚧 NO IMPLEMENTADO)");
  Serial.println("  pair_device                 → Emparejar dispositivo (🚧 NO IMPLEMENTADO)");
  Serial.println("  set_cross_mode <mode>       → Modo CROSS/BiCROSS (🚧 NO IMPLEMENTADO)");
  Serial.println("");
  
  Serial.println("📊 ALGORITMOS DISPONIBLES:");
  Serial.println("  list_algorithms             → Ver todos los algoritmos (✅ IMPLEMENTADO)");
  Serial.println("");
  
  Serial.println("🔍 LEYENDA:");
  Serial.println("  ✅ IMPLEMENTADO  - Funciona completamente");
  Serial.println("  🚧 NO IMPLEMENTADO - Responde apropiadamente, pendiente desarrollo");
  Serial.println("════════════════════════════════════════════════════════════");
}

// ==================== COMANDOS DEL SISTEMA ====================

static void show_status() {
  Serial.println("\n📊 ESTADO COMPLETO DEL SISTEMA");
  Serial.println("════════════════════════════════════════════════════════════");
  
  // Estado básico
  Serial.printf("🎧 Aurivox v3.0 - %s\n", audio_processing_active ? "ACTIVO" : "SLEEP");
  Serial.printf("💾 RAM libre: %d bytes\n", ESP.getFreeHeap());
  Serial.printf("💾 RAM mínima: %d bytes\n", ESP.getMinFreeHeap());
  Serial.printf("⚡ Procesamiento: %s\n", audio_processing_active ? "ACTIVO" : "INACTIVO");
  Serial.printf("💤 Sleep mode: %s\n", system_sleeping ? "SÍ" : "NO");
  Serial.printf("🔔 Pips activos: %s\n", are_pips_active() ? "SÍ" : "NO");
  Serial.println("");
  
  // Hardware
  Serial.println("🔧 HARDWARE:");
  Serial.printf("   I2S Hardware: %s\n", is_i2s_hardware_ready() ? "✅ LISTO" : "❌ ERROR");
  Serial.printf("   Audio Streams: %s\n", are_audio_streams_running() ? "✅ EJECUTANDO" : "❌ DETENIDOS");
  Serial.println("");
  
  // Audio actual
  Serial.println("🔊 AUDIO ACTUAL:");
  Serial.printf("   Ganancia: %.0f%% (Nivel %d/5)\n", gain_factor * 100, current_gain_level + 1);
  Serial.println("");
  
  // Algoritmos (estado futuro)
  Serial.println("🎛️ ALGORITMOS DSP:");
  Serial.printf("   Filtro Pasa-Altos: %s (%.0fHz)\n", 
                current_config.highpass_enabled ? "🚧 NO IMPL." : "❌ DESACTIVADO", 
                current_config.highpass_freq);
  Serial.printf("   Ecualizador: %s\n", 
                current_config.eq_enabled ? "🚧 NO IMPL." : "❌ DESACTIVADO");
  Serial.printf("   WDRC: %s (%.1fdB, %.1f:1)\n", 
                current_config.wdrc_enabled ? "🚧 NO IMPL." : "❌ DESACTIVADO",
                current_config.wdrc_threshold, current_config.wdrc_ratio);
  Serial.printf("   Limitador: %s (%.1fdB)\n", 
                current_config.limiter_enabled ? "🚧 NO IMPL." : "❌ DESACTIVADO",
                current_config.limiter_threshold);
  Serial.println("");
  
  Serial.println("📱 CONECTIVIDAD:");
  Serial.println("   Bluetooth A2DP: 🚧 NO IMPLEMENTADO");
  Serial.println("   CROSS/BiCROSS: 🚧 NO IMPLEMENTADO");
  Serial.println("   WiFi Control: 🚧 NO IMPLEMENTADO");
  Serial.println("");
  
  Serial.println("💾 CONFIGURACIÓN:");
  Serial.printf("   NVS: %s\n", nvs_initialized ? "✅ INICIALIZADO" : "❌ ERROR");
  Serial.printf("   Versión config: %d\n", current_config.version);
  Serial.println("════════════════════════════════════════════════════════════");
}

static void reset_to_default() {
  current_config = default_config;
  sync_config_to_system();
  Serial.println("✅ Sistema restaurado a configuración por defecto");
  Serial.println("⚠️ Cambios en RAM - usa 'save_preset default' para hacer permanente");
  show_status();
}

static void show_performance() {
  Serial.println("\n⚡ MÉTRICAS DE RENDIMIENTO");
  Serial.println("════════════════════════════════════════════════════════════");
  
  // Llamar función de hardware para métricas I2S
  get_audio_performance_info();
  
  // Métricas de memoria
  Serial.println("💾 MEMORIA:");
  Serial.printf("   RAM libre actual: %d bytes\n", ESP.getFreeHeap());
  Serial.printf("   RAM mínima histórica: %d bytes\n", ESP.getMinFreeHeap());
  Serial.printf("   RAM usada: %d bytes\n", 
                ESP.getHeapSize() - ESP.getFreeHeap());
  Serial.printf("   Fragmentación: %.1f%%\n", 
                100.0 - (100.0 * ESP.getMaxAllocHeap() / ESP.getFreeHeap()));
  
  // Estado de tareas (información general)
  Serial.println("");
  Serial.println("🎯 TAREAS DUAL-CORE:");
  Serial.println("   Core 0: Procesamiento de audio");
  Serial.println("   Core 1: Control y comunicación");
  Serial.printf("   Audio activo: %s\n", audio_processing_active ? "SÍ" : "NO");
  
  Serial.println("════════════════════════════════════════════════════════════");
}

static void run_full_diagnose() {
  Serial.println("\n🔍 DIAGNÓSTICO COMPLETO DEL SISTEMA");
  Serial.println("════════════════════════════════════════════════════════════");
  
  // Diagnóstico de hardware I2S
  diagnose_i2s_hardware();
  
  // Diagnóstico de botones
  get_button_status();
  
  Serial.println("💾 DIAGNÓSTICO NVS:");
  Serial.printf("   Inicializado: %s\n", nvs_initialized ? "✅" : "❌");
  if (nvs_initialized) {
    Serial.println("   Probando lectura/escritura...");
    if (save_config_to_nvs("test_diag")) {
      Serial.println("   ✅ Escritura NVS funcionando");
      if (load_config_from_nvs("test_diag")) {
        Serial.println("   ✅ Lectura NVS funcionando");
      }
    }
  }
  
  Serial.println("\n🎯 PRÓXIMOS MÓDULOS A IMPLEMENTAR:");
  Serial.println("   1. 🎛️ Filtros ESP-DSP (pasa-altos)");
  Serial.println("   2. 🎵 Ecualizador 6 bandas");
  Serial.println("   3. 🎚️ WDRC (compresión dinámica)");
  Serial.println("   4. 🛡️ Limitador anti-clipping");
  Serial.println("   5. 📱 Conectividad Bluetooth");
  Serial.println("   6. 🏥 Sistema médico completo");
  
  Serial.println("════════════════════════════════════════════════════════════");
}

// ==================== PROCESAMIENTO DE COMANDOS ====================

void handle_serial_commands() {
  if (!Serial.available()) return;
  
  String input = Serial.readStringUntil('\n');
  input.trim();
  input.toLowerCase();
  
  if (input.length() == 0) return;
  
  // Separar comando y parámetros
  int space_index = input.indexOf(' ');
  String command = input;
  String param = "";
  String param2 = "";
  
  if (space_index > 0) {
    command = input.substring(0, space_index);
    String remaining = input.substring(space_index + 1);
    int space_index2 = remaining.indexOf(' ');
    if (space_index2 > 0) {
      param = remaining.substring(0, space_index2);
      param2 = remaining.substring(space_index2 + 1);
    } else {
      param = remaining;
    }
  }
  
  Serial.printf("\n🎤 Comando: '%s'", command.c_str());
  if (param.length() > 0) Serial.printf(" | Parámetro: '%s'", param.c_str());
  if (param2.length() > 0) Serial.printf(" | Parámetro2: '%s'", param2.c_str());
  Serial.println();
  
  // ==================== COMANDOS BÁSICOS ====================
  
  if (command == "help" || command == "h") {
    show_help();
    
  } else if (command == "status" || command == "info") {
    show_status();
    
  } else if (command == "reset") {
    reset_to_default();
    
  } else if (command == "performance") {
    show_performance();
    
  } else if (command == "diagnose") {
    run_full_diagnose();
    
  } else if (command == "test_buttons") {
    test_button_system();
    
  // ==================== COMANDOS DE GANANCIA (IMPLEMENTADOS) ====================
  
  } else if (command == "set_gain_level") {
    if (param.length() == 0) {
      Serial.println("❌ Error: Especifica nivel 1-5");
    } else {
      int level = param.toInt();
      if (level >= 1 && level <= 5) {
        current_gain_level = level - 1;
        gain_factor = gain_levels[current_gain_level];
        Serial.printf("✅ Ganancia ajustada: %.0f%% (Nivel %d/5)\n", 
                      gain_factor * 100, current_gain_level + 1);
        // Detener pips si están activos para evitar interferencia
        force_stop_pips();
      } else {
        Serial.println("❌ Error: Nivel debe ser 1-5");
      }
    }
    
  } else if (command == "get_gain_level") {
    Serial.printf("🔊 Ganancia actual: %.0f%% (Nivel %d/5)\n", 
                  gain_factor * 100, current_gain_level + 1);
    
  // ==================== COMANDOS DE CONFIGURACIÓN (IMPLEMENTADOS) ====================
  
  } else if (command == "save_preset") {
    if (param.length() == 0) {
      Serial.println("❌ Error: Especifica nombre del preset");
      Serial.println("   Ejemplo: save_preset mi_config");
    } else {
      if (save_config_to_nvs(param.c_str())) {
        Serial.printf("💾 Configuración actual guardada como '%s'\n", param.c_str());
      }
    }
    
  } else if (command == "load_preset") {
    if (param.length() == 0) {
      Serial.println("❌ Error: Especifica nombre del preset");
      Serial.println("   Ejemplo: load_preset mi_config");
    } else {
      if (load_config_from_nvs(param.c_str())) {
        Serial.printf("📂 Configuración '%s' cargada y aplicada\n", param.c_str());
      }
    }
    
  } else if (command == "list_presets") {
    Serial.println("\n💾 CONFIGURACIONES GUARDADAS:");
    Serial.println("════════════════════════════════════════");
    
    nvs_iterator_t it = NULL;
    esp_err_t find_err = nvs_entry_find("nvs", "audio_config", NVS_TYPE_BLOB, &it);
    bool found_any = false;
    
    if (find_err == ESP_OK && it != NULL) {
      do {
        nvs_entry_info_t info;
        nvs_entry_info(it, &info);
        Serial.printf("📄 %s\n", info.key);
        found_any = true;
        
        esp_err_t next_err = nvs_entry_next(&it);
        if (next_err != ESP_OK) {
          break;
        }
      } while (it != NULL);
      
      // Liberar iterator
      if (it) {
        nvs_release_iterator(it);
      }
    }
    
    if (!found_any) {
      Serial.println("   (No hay configuraciones guardadas)");
    }
    Serial.println("════════════════════════════════════════");
    
  } else if (command == "delete_preset") {
    if (param.length() == 0) {
      Serial.println("❌ Error: Especifica nombre del preset");
    } else if (param == "default") {
      Serial.println("❌ Error: No se puede eliminar la configuración 'default'");
    } else {
      esp_err_t err = nvs_erase_key(nvs_config_handle, param.c_str());
      if (err == ESP_OK) {
        nvs_commit(nvs_config_handle);
        Serial.printf("🗑️ Configuración '%s' eliminada\n", param.c_str());
      } else if (err == ESP_ERR_NVS_NOT_FOUND) {
        Serial.printf("❌ Configuración '%s' no existe\n", param.c_str());
      } else {
        Serial.printf("❌ Error eliminando '%s': %s\n", param.c_str(), esp_err_to_name(err));
      }
    }
    
  } else if (command == "export_config") {
    Serial.println("\n📤 CONFIGURACIÓN ACTUAL (formato software):");
    Serial.println("CONFIG_START");
    Serial.printf("gain_level=%d\n", current_gain_level + 1);
    Serial.printf("highpass_enabled=%d\n", current_config.highpass_enabled ? 1 : 0);
    Serial.printf("highpass_freq=%.1f\n", current_config.highpass_freq);
    Serial.printf("eq_enabled=%d\n", current_config.eq_enabled ? 1 : 0);
    for (int i = 0; i < 6; i++) {
      Serial.printf("eq_band_%d=%.1f\n", i + 1, current_config.eq_gains[i]);
    }
    Serial.printf("wdrc_enabled=%d\n", current_config.wdrc_enabled ? 1 : 0);
    Serial.printf("wdrc_threshold=%.1f\n", current_config.wdrc_threshold);
    Serial.printf("wdrc_ratio=%.1f\n", current_config.wdrc_ratio);
    Serial.printf("limiter_enabled=%d\n", current_config.limiter_enabled ? 1 : 0);
    Serial.printf("limiter_threshold=%.1f\n", current_config.limiter_threshold);
    Serial.println("CONFIG_END");
    
  } else if (command == "list_algorithms") {
    Serial.println("\n🧮 ALGORITMOS DISPONIBLES:");
    Serial.println("════════════════════════════════════════");
    Serial.println("✅ IMPLEMENTADOS:");
    Serial.println("   🔊 Control de Ganancia (5 niveles)");
    Serial.println("   💾 Sistema de Configuración Persistente");
    Serial.println("   🔘 Control por Botones + Pips");
    Serial.println("   💤 Sleep Mode con Wake-up");
    Serial.println("");
    Serial.println("🚧 EN DESARROLLO:");
    Serial.println("   🎛️ Filtro Pasa-Altos (ESP-DSP)");
    Serial.println("   🎵 Ecualizador 6 Bandas (250Hz-8kHz)");
    Serial.println("   🎚️ WDRC (Wide Dynamic Range Compression)");
    Serial.println("   🛡️ Limitador Anti-Clipping");
    Serial.println("");
    Serial.println("📅 FUTUROS:");
    Serial.println("   🔇 Expansor/Gate de Ruido");
    Serial.println("   🎯 Anti-Feedback Adaptativo");
    Serial.println("   📱 Bluetooth A2DP + CROSS/BiCROSS");
    Serial.println("   🏥 Sistema Médico Completo");
    Serial.println("════════════════════════════════════════");
    
  // ==================== COMANDOS NO IMPLEMENTADOS (RESPUESTAS APROPIADAS) ====================
  
  } else if (command.startsWith("enable_") || command.startsWith("disable_") || 
             command.startsWith("set_") || command.startsWith("get_")) {
    Serial.printf("🚧 Comando '%s' reconocido pero NO IMPLEMENTADO aún\n", command.c_str());
    Serial.println("💡 Este comando está planificado para futuras versiones");
    Serial.println("📋 Usa 'list_algorithms' para ver el estado de desarrollo");
    
  } else {
    Serial.printf("❌ Comando desconocido: '%s'\n", command.c_str());
    Serial.println("💡 Escribe 'help' para ver comandos disponibles");
  }
  
  Serial.println(); // Línea en blanco para claridad
}

// ==================== FUNCIONES PÚBLICAS ====================

void initialize_serial_interface() {
  Serial.println("💬 INICIALIZANDO INTERFAZ SERIAL");
  Serial.println("─────────────────────────────────");
  
  // Inicializar NVS
  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    err = nvs_flash_init();
  }
  
  if (err == ESP_OK) {
    err = nvs_open("audio_config", NVS_READWRITE, &nvs_config_handle);
    if (err == ESP_OK) {
      nvs_initialized = true;
      Serial.println("✅ NVS inicializado correctamente");
      
      // Intentar cargar configuración default
      if (!load_config_from_nvs("default")) {
        Serial.println("📄 Creando configuración default inicial...");
        current_config = default_config;
        sync_config_to_system();
        save_config_to_nvs("default");
      }
    } else {
      Serial.printf("❌ Error abriendo NVS: %s\n", esp_err_to_name(err));
    }
  } else {
    Serial.printf("❌ Error inicializando NVS: %s\n", esp_err_to_name(err));
  }
  
  if (!nvs_initialized) {
    Serial.println("⚠️ NVS no disponible - configuración no persistente");
    current_config = default_config;
    sync_config_to_system();
  }
  
  Serial.println("─────────────────────────────────");
  Serial.println("✅ INTERFAZ SERIAL INICIALIZADA");
  Serial.printf("📋 Comandos disponibles: %d+ implementados\n", 20);
  Serial.println("💡 Escribe 'help' para ver todos los comandos");
}