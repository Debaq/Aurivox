// ==================== AUDIO_HARDWARE.CPP ====================
// Configuración y manejo de hardware I2S para Aurivox v3.0
// Hardware: XIAO ESP32S3 + micrófono ICS-43434 + DAC MAX98357A
// Configuración estable que no se modificará frecuentemente

#include "driver/i2s.h"
#include "esp_err.h"
#include "Arduino.h"

// ==================== DEFINICIONES DE PINES ====================

// Pines del micrófono ICS-43434
#define I2S_MIC_BCLK    2   // D2 - Bit Clock
#define I2S_MIC_LRCL    4   // D4 - Left/Right Clock (Word Select)
#define I2S_MIC_DOUT    5   // D5 - Data Output del micrófono

// Pines del DAC MAX98357A
#define I2S_DAC_BCLK    6   // D6 - Bit Clock
#define I2S_DAC_LRC     7   // D7 - Left/Right Clock (Word Select)
#define I2S_DAC_DIN     8   // D8 - Data Input al DAC

// ==================== CONFIGURACIONES DE AUDIO ====================

// Parámetros de audio (referenciados desde main)
extern const int SAMPLE_RATE;      // 16000 Hz
extern const int BUFFER_SIZE;      // 128 muestras
extern const i2s_port_t I2S_PORT_MIC;  // I2S_NUM_0
extern const i2s_port_t I2S_PORT_DAC;  // I2S_NUM_1

// ==================== VARIABLES DE ESTADO ====================

static bool i2s_hardware_initialized = false;
static bool audio_streams_running = false;

// ==================== FUNCIONES PRIVADAS ====================

// Configurar el micrófono I2S
static esp_err_t configure_microphone() {
    Serial.println("🎤 Configurando micrófono ICS-43434...");

    // Configuración I2S para micrófono (32-bit input)
    i2s_config_t i2s_config_mic = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,  // Micrófono da 32-bit
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,   // Solo canal izquierdo
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,      // Prioridad alta para audio
        .dma_buf_count = 2,                            // 2 buffers DMA
        .dma_buf_len = BUFFER_SIZE,                    // 128 muestras por buffer
        .use_apll = false,                             // No usar APLL para estabilidad
        .tx_desc_auto_clear = false,                   // No auto-clear (solo RX)
        .fixed_mclk = 0,                              // MCLK automático
        .mclk_multiple = I2S_MCLK_MULTIPLE_256,       // MCLK = 256 * sample_rate
        .bits_per_chan = I2S_BITS_PER_CHAN_32BIT      // 32 bits por canal
    };

    // Configuración de pines para micrófono
    i2s_pin_config_t pin_config_mic = {
        .mck_io_num = I2S_PIN_NO_CHANGE,    // No MCLK
        .bck_io_num = I2S_MIC_BCLK,         // D2 - Bit Clock
        .ws_io_num = I2S_MIC_LRCL,          // D4 - Word Select
        .data_out_num = I2S_PIN_NO_CHANGE,  // No hay salida de datos
        .data_in_num = I2S_MIC_DOUT         // D5 - Entrada de datos
    };

    // Instalar driver I2S para micrófono
    esp_err_t err = i2s_driver_install(I2S_PORT_MIC, &i2s_config_mic, 0, NULL);
    if (err != ESP_OK) {
        Serial.printf("❌ Error instalando driver micrófono: %s\n", esp_err_to_name(err));
        return err;
    }

    // Configurar pines
    err = i2s_set_pin(I2S_PORT_MIC, &pin_config_mic);
    if (err != ESP_OK) {
        Serial.printf("❌ Error configurando pines micrófono: %s\n", esp_err_to_name(err));
        i2s_driver_uninstall(I2S_PORT_MIC);
        return err;
    }

    Serial.println("✅ Micrófono ICS-43434 configurado correctamente");
    Serial.printf("   📍 BCLK: D%d, LRCL: D%d, DOUT: D%d\n", I2S_MIC_BCLK, I2S_MIC_LRCL, I2S_MIC_DOUT);

    return ESP_OK;
}

// Configurar el DAC I2S
static esp_err_t configure_dac() {
    Serial.println("🔊 Configurando DAC MAX98357A...");

    // Configuración I2S para DAC (16-bit output)
    i2s_config_t i2s_config_dac = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,  // DAC recibe 16-bit
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,   // Solo canal izquierdo
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,      // Prioridad alta para audio
        .dma_buf_count = 2,                            // 2 buffers DMA
        .dma_buf_len = BUFFER_SIZE,                    // 128 muestras por buffer
        .use_apll = false,                             // No usar APLL para estabilidad
        .tx_desc_auto_clear = true,                    // Auto-clear para TX
        .fixed_mclk = 0,                              // MCLK automático
        .mclk_multiple = I2S_MCLK_MULTIPLE_256,       // MCLK = 256 * sample_rate
        .bits_per_chan = I2S_BITS_PER_CHAN_16BIT      // 16 bits por canal
    };

    // Configuración de pines para DAC
    i2s_pin_config_t pin_config_dac = {
        .mck_io_num = I2S_PIN_NO_CHANGE,    // No MCLK
        .bck_io_num = I2S_DAC_BCLK,         // D6 - Bit Clock
        .ws_io_num = I2S_DAC_LRC,           // D7 - Word Select
        .data_out_num = I2S_DAC_DIN,        // D8 - Salida de datos
        .data_in_num = I2S_PIN_NO_CHANGE    // No hay entrada de datos
    };

    // Instalar driver I2S para DAC
    esp_err_t err = i2s_driver_install(I2S_PORT_DAC, &i2s_config_dac, 0, NULL);
    if (err != ESP_OK) {
        Serial.printf("❌ Error instalando driver DAC: %s\n", esp_err_to_name(err));
        return err;
    }

    // Configurar pines
    err = i2s_set_pin(I2S_PORT_DAC, &pin_config_dac);
    if (err != ESP_OK) {
        Serial.printf("❌ Error configurando pines DAC: %s\n", esp_err_to_name(err));
        i2s_driver_uninstall(I2S_PORT_DAC);
        return err;
    }

    Serial.println("✅ DAC MAX98357A configurado correctamente");
    Serial.printf("   📍 BCLK: D%d, LRC: D%d, DIN: D%d\n", I2S_DAC_BCLK, I2S_DAC_LRC, I2S_DAC_DIN);

    return ESP_OK;
}

// ==================== FUNCIONES PÚBLICAS ====================

// Inicializar todo el hardware I2S
void initialize_i2s_hardware() {
    if (i2s_hardware_initialized) {
        Serial.println("⚠️ Hardware I2S ya inicializado");
        return;
    }

    Serial.println("🔧 INICIALIZANDO HARDWARE I2S");
    Serial.println("────────────────────────────────");
    Serial.printf("📊 Sample Rate: %d Hz\n", SAMPLE_RATE);
    Serial.printf("📦 Buffer Size: %d muestras\n", BUFFER_SIZE);
    Serial.printf("⏱️ Latencia base: %.1f ms\n", (float)BUFFER_SIZE / SAMPLE_RATE * 1000);

    // Configurar micrófono
    esp_err_t mic_result = configure_microphone();
    if (mic_result != ESP_OK) {
        Serial.println("❌ FALLO EN CONFIGURACIÓN DE MICRÓFONO");
        return;
    }

    // Configurar DAC
    esp_err_t dac_result = configure_dac();
    if (dac_result != ESP_OK) {
        Serial.println("❌ FALLO EN CONFIGURACIÓN DE DAC");
        // Desinstalar micrófono si el DAC falla
        i2s_driver_uninstall(I2S_PORT_MIC);
        return;
    }

    i2s_hardware_initialized = true;
    Serial.println("────────────────────────────────");
    Serial.println("✅ HARDWARE I2S INICIALIZADO CORRECTAMENTE");
    Serial.printf("💾 RAM libre después de init: %d bytes\n", ESP.getFreeHeap());
}

// Iniciar streams de audio
void start_audio_streams() {
    if (!i2s_hardware_initialized) {
        Serial.println("❌ Hardware no inicializado - no se pueden iniciar streams");
        return;
    }

    if (audio_streams_running) {
        Serial.println("⚠️ Streams de audio ya están ejecutándose");
        return;
    }

    Serial.println("🚀 Iniciando streams de audio...");

    // Iniciar micrófono
    esp_err_t mic_start = i2s_start(I2S_PORT_MIC);
    if (mic_start != ESP_OK) {
        Serial.printf("❌ Error iniciando micrófono: %s\n", esp_err_to_name(mic_start));
        return;
    }

    // Iniciar DAC
    esp_err_t dac_start = i2s_start(I2S_PORT_DAC);
    if (dac_start != ESP_OK) {
        Serial.printf("❌ Error iniciando DAC: %s\n", esp_err_to_name(dac_start));
        i2s_stop(I2S_PORT_MIC);  // Detener micrófono si DAC falla
        return;
    }

    audio_streams_running = true;
    Serial.println("✅ Streams de audio iniciados correctamente");
    Serial.println("🎵 Audio en tiempo real activo");
}

// Detener streams de audio
void stop_audio_streams() {
    if (!audio_streams_running) {
        Serial.println("⚠️ Streams de audio ya están detenidos");
        return;
    }

    Serial.println("⏹️ Deteniendo streams de audio...");

    // Detener streams
    i2s_stop(I2S_PORT_MIC);
    i2s_stop(I2S_PORT_DAC);

    audio_streams_running = false;
    Serial.println("✅ Streams de audio detenidos");
}

// Reiniciar streams de audio (útil para sleep/wake)
void restart_audio_streams() {
    Serial.println("🔄 Reiniciando streams de audio...");
    stop_audio_streams();
    delay(100);  // Pequeña pausa para estabilidad
    start_audio_streams();
}

// Verificar estado del hardware
bool is_i2s_hardware_ready() {
    return i2s_hardware_initialized;
}

// Verificar estado de los streams
bool are_audio_streams_running() {
    return audio_streams_running;
}

// Obtener información de rendimiento
void get_audio_performance_info() {
    Serial.println("\n📊 INFORMACIÓN DE RENDIMIENTO I2S");
    Serial.println("════════════════════════════════════");
    Serial.printf("🔧 Hardware inicializado: %s\n", i2s_hardware_initialized ? "SÍ" : "NO");
    Serial.printf("🎵 Streams ejecutándose: %s\n", audio_streams_running ? "SÍ" : "NO");
    Serial.printf("📊 Sample Rate: %d Hz\n", SAMPLE_RATE);
    Serial.printf("📦 Buffer Size: %d muestras\n", BUFFER_SIZE);
    Serial.printf("⏱️ Latencia teórica: %.1f ms\n", (float)BUFFER_SIZE / SAMPLE_RATE * 1000);
    Serial.printf("🎤 Puerto micrófono: I2S_%d\n", I2S_PORT_MIC);
    Serial.printf("🔊 Puerto DAC: I2S_%d\n", I2S_PORT_DAC);
    Serial.printf("💾 RAM libre: %d bytes\n", ESP.getFreeHeap());
    Serial.println("════════════════════════════════════");
}

// Función de diagnóstico
void diagnose_i2s_hardware() {
    Serial.println("\n🔍 DIAGNÓSTICO DE HARDWARE I2S");
    Serial.println("═══════════════════════════════════");

    // Verificar configuración de pines
    Serial.println("📍 CONFIGURACIÓN DE PINES:");
    Serial.printf("   Micrófono - BCLK: D%d, LRCL: D%d, DOUT: D%d\n",
                  I2S_MIC_BCLK, I2S_MIC_LRCL, I2S_MIC_DOUT);
    Serial.printf("   DAC - BCLK: D%d, LRC: D%d, DIN: D%d\n",
                  I2S_DAC_BCLK, I2S_DAC_LRC, I2S_DAC_DIN);

    // Verificar estado de drivers
    Serial.println("\n🔧 ESTADO DE DRIVERS:");
    Serial.printf("   Hardware inicializado: %s\n", i2s_hardware_initialized ? "✅" : "❌");
    Serial.printf("   Streams activos: %s\n", audio_streams_running ? "✅" : "❌");

    // Información de memoria
    Serial.println("\n💾 MEMORIA:");
    Serial.printf("   RAM libre: %d bytes\n", ESP.getFreeHeap());
    Serial.printf("   RAM mínima libre: %d bytes\n", ESP.getMinFreeHeap());

    // Recomendaciones
    Serial.println("\n💡 RECOMENDACIONES:");
    if (!i2s_hardware_initialized) {
        Serial.println("   ⚠️ Ejecutar initialize_i2s_hardware() primero");
    }
    if (i2s_hardware_initialized && !audio_streams_running) {
        Serial.println("   ⚠️ Ejecutar start_audio_streams() para audio");
    }
    if (ESP.getFreeHeap() < 50000) {
        Serial.println("   ⚠️ Memoria RAM baja - posibles problemas de rendimiento");
    }

    Serial.println("═══════════════════════════════════");
}
