// ==================== BUTTON_CONTROL.CPP ====================
// Sistema de control de botones y feedback de audio para Aurivox v3.0
// Versión limpia - Manejo de ISRs, debounce, sleep y sistema de pips

#include "Arduino.h"
#include "esp_sleep.h"
#include "driver/rtc_io.h"
#include "audio_config.h"
#include "button_control.h"  // Para acceder a los tipos definidos en el header

// ==================== DEFINICIONES DE PINES ====================

#define BTN_GAIN_UP     3   // D3 - Botón subir ganancia
#define BTN_GAIN_DOWN   0   // D0 - Botón bajar ganancia  
#define BTN_SLEEP       1   // D1 - Botón sleep/wake (mantener 3s)

// ==================== CONFIGURACIONES DE DEBOUNCE ====================

#define DEBOUNCE_DELAY      50    // 50ms debounce
#define SLEEP_HOLD_TIME     3000  // 3 segundos para activar sleep

// ==================== VARIABLES GLOBALES DE BOTONES ====================

// Variables volátiles para ISRs
volatile bool btn_gain_up_flag = false;
volatile bool btn_gain_down_flag = false;
volatile bool btn_sleep_flag = false;
volatile unsigned long last_interrupt_time = 0;

// Variables de control de sleep
static unsigned long btn_sleep_press_start = 0;
static bool btn_sleep_held = false;
static bool sleep_sequence_started = false;

// Referencias a variables globales del sistema
extern volatile bool audio_processing_active;
extern volatile bool system_sleeping;
extern volatile int current_gain_level;
extern volatile float gain_factor;
extern const float gain_levels[5];

// Declaraciones de funciones externas
extern void stop_audio_streams();
extern void start_audio_streams();

// Declaración de función interna
static void enter_sleep_mode();

// ==================== SISTEMA DE PIPS ====================

// Usar la estructura PipSystem definida en audio_config.h
static PipSystem pip_system = {false, 0, 0, 0, 0, 0, false, 0.0};

// Iniciar secuencia de pips
static void start_pip_sequence(int num_pips) {
    pip_system.active = true;
    pip_system.total_pips = num_pips;
    pip_system.remaining_pips = num_pips;
    pip_system.samples_in_current_pip = PIP_SAMPLES;
    pip_system.pip_start_time = millis();
    pip_system.in_gap = false;
    pip_system.phase = 0.0;
    
    Serial.printf("🔔 Iniciando secuencia de %d pips\n", num_pips);
}

// Detener sistema de pips
static void stop_pip_sequence() {
    pip_system.active = false;
    pip_system.remaining_pips = 0;
    pip_system.in_gap = false;
    pip_system.phase = 0.0;
    Serial.println("🔔 Pips detenidos");
}

// ==================== ISRs OPTIMIZADAS ====================

void IRAM_ATTR btn_gain_up_isr() {
    unsigned long interrupt_time = millis();
    if (interrupt_time - last_interrupt_time > DEBOUNCE_DELAY) {
        btn_gain_up_flag = true;
        last_interrupt_time = interrupt_time;
    }
}

void IRAM_ATTR btn_gain_down_isr() {
    unsigned long interrupt_time = millis();
    if (interrupt_time - last_interrupt_time > DEBOUNCE_DELAY) {
        btn_gain_down_flag = true;
        last_interrupt_time = interrupt_time;
    }
}

void IRAM_ATTR btn_sleep_isr() {
    unsigned long interrupt_time = millis();
    if (interrupt_time - last_interrupt_time > DEBOUNCE_DELAY) {
        btn_sleep_flag = true;
        last_interrupt_time = interrupt_time;
    }
}

// ==================== MANEJO DE BOTONES DE GANANCIA ====================

static void handle_gain_buttons() {
    // Botón subir ganancia
    if (btn_gain_up_flag) {
        btn_gain_up_flag = false;
        
        if (current_gain_level < 4) {
            current_gain_level++;
            gain_factor = gain_levels[current_gain_level];
            Serial.printf("🔊 Ganancia: %.0f%% (Nivel %d/5)\n", 
                          gain_factor * 100, current_gain_level + 1);
            
            // Pip de confirmación (1 pip por cada nivel)
            start_pip_sequence(current_gain_level + 1);
        } else {
            Serial.println("🔊 Ganancia al MÁXIMO");
            start_pip_sequence(3);  // 3 pips para indicar máximo
        }
    }
    
    // Botón bajar ganancia
    if (btn_gain_down_flag) {
        btn_gain_down_flag = false;
        
        if (current_gain_level > 0) {
            current_gain_level--;
            gain_factor = gain_levels[current_gain_level];
            Serial.printf("🔉 Ganancia: %.0f%% (Nivel %d/5)\n", 
                          gain_factor * 100, current_gain_level + 1);
            
            // Pip de confirmación (1 pip por cada nivel, mínimo 1)
            start_pip_sequence(current_gain_level + 1);
        } else {
            Serial.println("🔉 Ganancia al MÍNIMO");
            start_pip_sequence(1);  // 1 pip para indicar mínimo
        }
    }
}

// ==================== MANEJO DE BOTÓN SLEEP ====================

static void handle_sleep_button() {
    // Verificar estado actual del botón
    bool btn_currently_pressed = (digitalRead(BTN_SLEEP) == LOW);
    
    if (btn_sleep_flag) {
        btn_sleep_flag = false;
        
        if (btn_currently_pressed && !btn_sleep_held) {
            // Botón recién presionado
            btn_sleep_press_start = millis();
            btn_sleep_held = true;
            sleep_sequence_started = false;
            Serial.println("💤 Botón sleep presionado - mantener 3 segundos...");
            
        } else if (!btn_currently_pressed && btn_sleep_held) {
            // Botón soltado antes de tiempo
            unsigned long press_duration = millis() - btn_sleep_press_start;
            btn_sleep_held = false;
            
            if (press_duration >= 1000 && press_duration < SLEEP_HOLD_TIME) {
                Serial.println("💤 Sleep CANCELADO (soltado antes de tiempo)");
                stop_pip_sequence();  // Cancelar pips si están sonando
            }
            
            sleep_sequence_started = false;
        }
    }
    
    // Lógica de hold continuo
    if (btn_sleep_held && btn_currently_pressed) {
        unsigned long press_duration = millis() - btn_sleep_press_start;
        
        if (press_duration >= 1000 && !sleep_sequence_started) {
            // 1 segundo - iniciar secuencia de aviso
            Serial.println("💤 Preparando sleep - 2 segundos más...");
            start_pip_sequence(3);  // 3 pips de aviso
            sleep_sequence_started = true;
            
        } else if (press_duration >= SLEEP_HOLD_TIME) {
            // 3 segundos - entrar en sleep
            Serial.println("💤 Entrando en LIGHT SLEEP...");
            enter_sleep_mode();
        }
    }
    
    // Reset si botón no está presionado
    if (!btn_currently_pressed) {
        btn_sleep_held = false;
        sleep_sequence_started = false;
    }
}

// ==================== FUNCIÓN DE SLEEP MODE ====================

static void enter_sleep_mode() {
    Serial.println("💤 Iniciando secuencia de sleep...");
    
    // Detener sistema de pips
    stop_pip_sequence();
    
    // Marcar sistema como durmiendo
    audio_processing_active = false;
    system_sleeping = true;
    
    Serial.println("💤 Deteniendo streams de audio...");
    stop_audio_streams();
    
    // Pequeña pausa para estabilidad
    delay(100);
    
    Serial.println("💤 Configurando wake-up por botón D1...");
    // Configurar wake-up solo por botón D1 (BTN_SLEEP)
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_1, 0);  // Wake up cuando D1 = LOW
    
    Serial.println("💤 Entrando en light sleep...");
    Serial.flush();  // Asegurar que se imprima antes del sleep
    
    // Entrar en light sleep
    esp_light_sleep_start();
    
    // ==================== AL DESPERTAR ====================
    
    Serial.println("⚡ DESPERTANDO del sleep...");
    
    // Restaurar estado del sistema
    system_sleeping = false;
    audio_processing_active = true;
    btn_sleep_held = false;
    sleep_sequence_started = false;
    
    Serial.println("⚡ Reiniciando streams de audio...");
    start_audio_streams();
    
    Serial.printf("⚡ Sistema restaurado - Ganancia: %.0f%% (Nivel %d/5)\n", 
                  gain_factor * 100, current_gain_level + 1);
    
    // Pip de confirmación de despertar
    start_pip_sequence(2);
}

// ==================== FUNCIONES PÚBLICAS ESENCIALES ====================

// Inicializar sistema de botones
void initialize_buttons() {
    Serial.println("🔘 INICIALIZANDO SISTEMA DE BOTONES");
    Serial.println("────────────────────────────────────");
    
    // Configurar pines como entrada con pull-up
    pinMode(BTN_GAIN_UP, INPUT_PULLUP);
    pinMode(BTN_GAIN_DOWN, INPUT_PULLUP);
    pinMode(BTN_SLEEP, INPUT_PULLUP);
    
    Serial.printf("📍 D%d: Subir ganancia\n", BTN_GAIN_UP);
    Serial.printf("📍 D%d: Bajar ganancia\n", BTN_GAIN_DOWN);
    Serial.printf("📍 D%d: Sleep (mantener 3s)\n", BTN_SLEEP);
    
    // Configurar interrupciones
    attachInterrupt(digitalPinToInterrupt(BTN_GAIN_UP), btn_gain_up_isr, FALLING);
    attachInterrupt(digitalPinToInterrupt(BTN_GAIN_DOWN), btn_gain_down_isr, FALLING);
    attachInterrupt(digitalPinToInterrupt(BTN_SLEEP), btn_sleep_isr, CHANGE);
    
    Serial.println("✅ ISRs configuradas con debounce");
    
    // Configurar wake-up para sleep mode
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_1, 0);
    Serial.println("✅ Wake-up configurado (botón D1)");
    
    Serial.println("────────────────────────────────────");
    Serial.println("✅ SISTEMA DE BOTONES INICIALIZADO");
}

// Manejar eventos de botones (llamada desde Core 1)
void handle_button_events() {
    if (system_sleeping) {
        return;  // No procesar botones si el sistema está durmiendo
    }
    
    handle_gain_buttons();
    handle_sleep_button();
}

// Procesar audio de pips (llamada desde Core 0)
bool process_pip_audio(int16_t* buffer, int num_samples) {
    if (!pip_system.active) {
        return false;  // No hay pips activos
    }
    
    const float phase_increment = 2.0 * PI * PIP_FREQUENCY / SAMPLE_RATE;
    
    for (int i = 0; i < num_samples; i++) {
        if (pip_system.in_gap) {
            // Silencio entre pips
            buffer[i] = 0;
            
            // Verificar si termina el gap
            if (millis() - pip_system.pip_gap_start >= PIP_GAP_MS) {
                pip_system.in_gap = false;
                pip_system.samples_in_current_pip = PIP_SAMPLES;
                pip_system.pip_start_time = millis();
            }
        } else {
            // Generar tono del pip
            if (pip_system.samples_in_current_pip > 0) {
                buffer[i] = (int16_t)(sin(pip_system.phase) * PIP_AMPLITUDE * 32767);
                pip_system.phase += phase_increment;
                if (pip_system.phase >= 2.0 * PI) {
                    pip_system.phase -= 2.0 * PI;
                }
                pip_system.samples_in_current_pip--;
            } else {
                // Termina este pip
                pip_system.remaining_pips--;
                if (pip_system.remaining_pips > 0) {
                    // Iniciar gap antes del siguiente pip
                    pip_system.in_gap = true;
                    pip_system.pip_gap_start = millis();
                    buffer[i] = 0;
                } else {
                    // Terminar secuencia completa
                    pip_system.active = false;
                    buffer[i] = 0;
                    Serial.printf("🔔 Secuencia de %d pips completada\n", pip_system.total_pips);
                }
            }
        }
    }
    
    return true;  // Se procesaron pips
}

// Verificar si hay pips activos
bool are_pips_active() {
    return pip_system.active;
}

// Forzar detener pips (útil para comandos)
void force_stop_pips() {
    if (pip_system.active) {
        stop_pip_sequence();
    }
}

// Obtener estado de botones para diagnóstico
void get_button_status() {
    Serial.println("\n🔘 ESTADO DEL SISTEMA DE BOTONES");
    Serial.println("════════════════════════════════════");
    Serial.printf("📍 D%d (Gain +): %s\n", BTN_GAIN_UP, 
                  digitalRead(BTN_GAIN_UP) ? "ALTO" : "BAJO");
    Serial.printf("📍 D%d (Gain -): %s\n", BTN_GAIN_DOWN, 
                  digitalRead(BTN_GAIN_DOWN) ? "ALTO" : "BAJO");
    Serial.printf("📍 D%d (Sleep): %s\n", BTN_SLEEP, 
                  digitalRead(BTN_SLEEP) ? "ALTO" : "BAJO");
    Serial.println("");
    Serial.printf("🎚️ Ganancia actual: %.0f%% (Nivel %d/5)\n", 
                  gain_factor * 100, current_gain_level + 1);
    Serial.printf("🔔 Pips activos: %s\n", pip_system.active ? "SÍ" : "NO");
    if (pip_system.active) {
        Serial.printf("   └─ Pips restantes: %d/%d\n", pip_system.remaining_pips, pip_system.total_pips);
        Serial.printf("   └─ En gap: %s\n", pip_system.in_gap ? "SÍ" : "NO");
    }
    Serial.printf("💤 Sistema durmiendo: %s\n", system_sleeping ? "SÍ" : "NO");
    Serial.printf("🎵 Audio activo: %s\n", audio_processing_active ? "SÍ" : "NO");
    Serial.println("════════════════════════════════════");
}

// Test de botones (para diagnóstico)
void test_button_system() {
    Serial.println("\n🧪 TEST DEL SISTEMA DE BOTONES");
    Serial.println("═══════════════════════════════════");
    Serial.println("Presiona cada botón para verificar:");
    Serial.printf("  D%d: Debería aumentar ganancia + pips\n", BTN_GAIN_UP);
    Serial.printf("  D%d: Debería disminuir ganancia + pips\n", BTN_GAIN_DOWN);
    Serial.printf("  D%d: Mantener 3s para sleep\n", BTN_SLEEP);
    Serial.println("");
    Serial.println("Test automático de pips:");
    
    for (int i = 1; i <= 5; i++) {
        Serial.printf("🔔 Probando %d pip(s)...\n", i);
        start_pip_sequence(i);
        
        // Esperar a que terminen los pips
        while (pip_system.active) {
            delay(50);
        }
        delay(500);  // Pausa entre tests
    }
    
    Serial.println("✅ Test de pips completado");
    Serial.println("═══════════════════════════════════");
}

// ==================== FUNCIONES STUB (NO IMPLEMENTADAS) ====================

// Control de ganancia
bool increment_gain_level() {
    Serial.println("🚧 increment_gain_level() - NO IMPLEMENTADO");
    return false;
}

bool decrement_gain_level() {
    Serial.println("🚧 decrement_gain_level() - NO IMPLEMENTADO");
    return false;
}

bool set_gain_level(int level) {
    Serial.printf("🚧 set_gain_level(%d) - NO IMPLEMENTADO\n", level);
    return false;
}

int get_current_gain_level() {
    return current_gain_level + 1;  // Convertir índice a nivel 1-5
}

float get_current_gain_factor() {
    return gain_factor;
}

// Sleep mode
void enter_sleep_mode_manual() {
    Serial.println("🚧 enter_sleep_mode_manual() - NO IMPLEMENTADO");
}

bool is_system_sleeping() {
    return system_sleeping;
}

bool set_sleep_hold_time(uint32_t hold_time_ms) {
    Serial.printf("🚧 set_sleep_hold_time(%lu) - NO IMPLEMENTADO\n", hold_time_ms);
    return false;
}

uint32_t get_sleep_hold_time() {
    return SLEEP_HOLD_TIME;
}

// Configuración de pips
bool configure_pip_system(float frequency, float amplitude, uint32_t duration_ms, uint32_t gap_ms) {
    Serial.println("🚧 configure_pip_system() - NO IMPLEMENTADO");
    return false;
}

void get_pip_configuration(float* frequency, float* amplitude, uint32_t* duration_ms, uint32_t* gap_ms) {
    Serial.println("🚧 get_pip_configuration() - NO IMPLEMENTADO");
}

bool play_custom_pip_sequence(int num_pips, float frequency, float amplitude) {
    Serial.printf("🚧 play_custom_pip_sequence(%d) - NO IMPLEMENTADO\n", num_pips);
    return false;
}

// Diagnóstico avanzado
bool verify_isr_integrity() {
    Serial.println("🚧 verify_isr_integrity() - NO IMPLEMENTADO");
    return false;
}

uint32_t measure_button_response_time(uint8_t button_pin, int num_samples) {
    Serial.println("🚧 measure_button_response_time() - NO IMPLEMENTADO");
    return 0;
}

// Configuración avanzada
bool set_debounce_time(uint32_t debounce_ms) {
    Serial.printf("🚧 set_debounce_time(%lu) - NO IMPLEMENTADO\n", debounce_ms);
    return false;
}

uint32_t get_debounce_time() {
    return DEBOUNCE_DELAY;
}

bool set_button_enabled(uint8_t button_pin, bool enabled) {
    Serial.println("🚧 set_button_enabled() - NO IMPLEMENTADO");
    return false;
}

bool is_button_enabled(uint8_t button_pin) {
    Serial.println("🚧 is_button_enabled() - NO IMPLEMENTADO");
    return true;
}

// Callbacks
void register_button_callback(button_callback_t callback) {
    Serial.println("🚧 register_button_callback() - NO IMPLEMENTADO");
}

// Información del sistema
void get_button_system_info(button_system_info_t* info) {
    Serial.println("🚧 get_button_system_info() - NO IMPLEMENTADO");
}

// Persistencia
bool save_button_config(const char* preset_name) {
    Serial.printf("🚧 save_button_config(%s) - NO IMPLEMENTADO\n", preset_name);
    return false;
}

bool load_button_config(const char* preset_name) {
    Serial.printf("🚧 load_button_config(%s) - NO IMPLEMENTADO\n", preset_name);
    return false;
}