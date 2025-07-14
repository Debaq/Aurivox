// ==================== AURIVOX v3.0 - ARCHIVO PRINCIPAL ====================
// Audífono digital profesional modular con ESP32S3 + ESP-DSP
// Hardware: XIAO ESP32S3, micrófono ICS-43434, DAC MAX98357A
// Arquitectura: Dual-core especializado + módulos separados

#include "driver/i2s.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_bt.h"

// ==================== CONFIGURACIONES GLOBALES ====================

// Audio settings
#define SAMPLE_RATE     16000
#define BUFFER_SIZE     128
#define I2S_PORT_MIC    I2S_NUM_0
#define I2S_PORT_DAC    I2S_NUM_1

// Buffers de audio
int32_t mic_buffer[BUFFER_SIZE];
int16_t dac_buffer[BUFFER_SIZE];

// Estado del sistema
volatile bool audio_processing_active = true;
volatile bool system_sleeping = false;

// Ganancia básica (será expandido por módulos)
volatile int current_gain_level = 2;
const float gain_levels[5] = {0.0, 0.25, 0.50, 0.75, 1.0};
volatile float gain_factor = 0.5;

// Handles para tareas dual-core
TaskHandle_t audioTaskHandle = NULL;
TaskHandle_t controlTaskHandle = NULL;

// ==================== DECLARACIONES DE FUNCIONES EXTERNAS ====================

// audio_hardware.cpp
extern void initialize_i2s_hardware();
extern void start_audio_streams();
extern void stop_audio_streams();

// button_control.cpp
extern void initialize_buttons();
extern void handle_button_events();
extern bool process_pip_audio(int16_t* buffer, int num_samples);

// serial_commands.cpp
extern void initialize_serial_interface();
extern void handle_serial_commands();

// ==================== TAREA CORE 0: PROCESAMIENTO DE AUDIO ====================

void audioTask(void* parameter) {
    Serial.println("🎵 Core 0: Tarea de audio iniciada");

    // Variables locales para la tarea de audio
    size_t bytes_read = 0;
    size_t bytes_written = 0;

    while (true) {
        // Solo procesar si el sistema está activo
        if (!audio_processing_active || system_sleeping) {
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        // Leer del micrófono
        esp_err_t read_result = i2s_read(I2S_PORT_MIC, mic_buffer, sizeof(mic_buffer), &bytes_read, pdMS_TO_TICKS(10));
        if (read_result != ESP_OK) {
            continue;
        }

        int num_samples = bytes_read / sizeof(int32_t);

        // Procesar cada muestra
        for (int i = 0; i < num_samples; i++) {
            int16_t sample_16;

            // Verificar si hay pips activos (del sistema de botones)
            if (process_pip_audio(&sample_16, 1)) {
                // Usar muestra del pip
                dac_buffer[i] = sample_16;
            } else {
                // Procesar audio normal del micrófono
                sample_16 = mic_buffer[i] >> 16;  // Convertir 32-bit a 16-bit

                // ==================== PIPELINE DSP (A IMPLEMENTAR) ====================
                // TODO: 1. Filtro Pasa-Altos (ESP-DSP)
                // TODO: 2. Ecualizador 6 Bandas (ESP-DSP)
                // TODO: 3. WDRC (Wide Dynamic Range Compression)
                // TODO: 4. Limitador Anti-Clipping

                // Por ahora: solo ganancia básica
                float sample_float = (float)sample_16 * gain_factor;

                // Limitación básica para evitar clipping
                if (sample_float > 32767.0) sample_float = 32767.0;
                if (sample_float < -32768.0) sample_float = -32768.0;

                dac_buffer[i] = (int16_t)sample_float;
            }
        }

        // Enviar al DAC
        i2s_write(I2S_PORT_DAC, dac_buffer, num_samples * sizeof(int16_t), &bytes_written, pdMS_TO_TICKS(10));

        // Yield para permitir otras tareas
        taskYIELD();
    }
}

// ==================== TAREA CORE 1: CONTROL Y COMUNICACIÓN ====================

void controlTask(void* parameter) {
    Serial.println("🎛️ Core 1: Tarea de control iniciada");

    while (true) {
        // Manejar comandos seriales
        handle_serial_commands();

        // Manejar eventos de botones
        handle_button_events();

        // Delay para no saturar el core
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

// ==================== SETUP ====================

void setup() {
    Serial.begin(115200);
    delay(2000);

    Serial.println("════════════════════════════════════════");
    Serial.println("🎧 AURIVOX v3.0 - INICIALIZANDO");
    Serial.println("════════════════════════════════════════");
    Serial.println("📡 Arquitectura: Dual-core ESP32S3");
    Serial.println("🎵 Core 0: Procesamiento de audio");
    Serial.println("🎛️ Core 1: Control y comunicación");
    Serial.println("════════════════════════════════════════");

    // DESHABILITAR WiFi y Bluetooth para máximo rendimiento
    esp_wifi_stop();
    esp_wifi_deinit();
    esp_bt_controller_disable();
    Serial.println("📡 WiFi/Bluetooth: DESHABILITADOS");

    // Inicializar módulos de hardware
    Serial.println("\n🔧 INICIALIZANDO HARDWARE:");
    initialize_i2s_hardware();
    initialize_buttons();
    initialize_serial_interface();

    Serial.println("\n🚀 CONFIGURANDO DUAL-CORE:");

    // Crear tarea de audio en Core 0 (dedicado a DSP)
    xTaskCreatePinnedToCore(
        audioTask,           // Función de la tarea
        "AudioTask",         // Nombre de la tarea
        4096,               // Tamaño del stack (4KB)
    NULL,               // Parámetro de la tarea
    2,                  // Prioridad (alta para audio)
    &audioTaskHandle,   // Handle de la tarea
    0                   // Core 0 (dedicado a audio)
    );

    // Crear tarea de control en Core 1 (para UI y comunicación)
    xTaskCreatePinnedToCore(
        controlTask,        // Función de la tarea
        "ControlTask",      // Nombre de la tarea
        4096,              // Tamaño del stack (4KB)
    NULL,              // Parámetro de la tarea
    1,                 // Prioridad (menor que audio)
    &controlTaskHandle, // Handle de la tarea
    1                  // Core 1 (para control)
    );

    Serial.println("✅ Core 0: Tarea de audio creada");
    Serial.println("✅ Core 1: Tarea de control creada");

    // Iniciar streams de audio
    start_audio_streams();

    Serial.println("\n🎯 SISTEMA LISTO");
    Serial.printf("💾 RAM libre: %d bytes\n", ESP.getFreeHeap());
    Serial.printf("🔊 Ganancia inicial: %.0f%% (Nivel %d/5)\n",
                  gain_factor * 100, current_gain_level + 1);
    Serial.println("💬 Escribe 'help' para ver comandos disponibles");
    Serial.println("════════════════════════════════════════");
}

// ==================== LOOP PRINCIPAL ====================

void loop() {
    // El loop principal está prácticamente vacío porque todo el trabajo
    // se hace en las tareas dual-core creadas en setup()

    // Solo mantener watchdog feliz y mostrar estado ocasionalmente
    static unsigned long last_status_time = 0;
    unsigned long current_time = millis();

    // Mostrar estado cada 30 segundos
    if (current_time - last_status_time > 30000) {
        Serial.printf("💓 Sistema activo - RAM libre: %d bytes\n", ESP.getFreeHeap());
        last_status_time = current_time;
    }

    // Delay pequeño para no consumir CPU innecesariamente
    delay(1000);
}
