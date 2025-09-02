#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <variant>
#include <memory>
#include <string>
#include <vector>

// =============================================================================
// HARDWARE-KONFIGURATION
// =============================================================================

// GPIO-Pins für M5Stack ATOM Echo
#define LED_PIN             27      // ARGB LED
#define BUTTON_PIN          39      // Taster

// I2S-Konfiguration
#define I2S_MIC_CHANNELS    1       // Mono-Mikrofon
#define I2S_SPEAKER_CHANNELS 1      // Mono-Lautsprecher
#define I2S_SAMPLE_RATE     16000   // 16kHz Sample Rate
#define I2S_BITS_PER_SAMPLE 16      // 16-bit Audio
#define I2S_CHANNELS        1       // Mono
#define I2S_BUFFER_SIZE     1024    // Audio-Buffer Größe

// I2S-PINS FÜR M5STACK ATOM ECHO (VERIFIZIERT)
#define I2S_MIC_BCK_PIN      19
#define I2S_MIC_WS_PIN       33
#define I2S_MIC_DATA_PIN     23

#define I2S_SPEAKER_BCK_PIN  19  // Teilt sich den Pin mit dem Mikrofon
#define I2S_SPEAKER_WS_PIN   33  // Teilt sich den Pin mit dem Mikrofon
#define I2S_SPEAKER_DATA_PIN 22

// I2S-Port-Konfiguration
#define I2S_MIC_PORT        I2S_NUM_0
#define I2S_SPEAKER_PORT    I2S_NUM_1

// LAUTSPRECHER-VERSTÄRKER-STEUERUNG (EXKLUSIV)
#define SPEAKER_ENABLE_PIN   25

// =============================================================================
// NETZWERK-KONFIGURATION
// =============================================================================

// Access Point Konfiguration
#define AP_SSID             "M5_Echo_Config"
#define AP_PASSWORD         "12345678"      // Mindestens 8 Zeichen für WPA2
#define AP_IP               "192.168.4.1"
#define AP_GATEWAY          "192.168.4.1"
#define AP_SUBNET           "255.255.255.0"
#define WIFI_TIMEOUT_MS    10000           // 10 Sekunden Timeout für WLAN-Verbindung
#define WIFI_RECONNECT_INTERVAL 30000      // 30 Sekunden zwischen Reconnect-Versuchen
#define WIFI_MAX_RECONNECT_ATTEMPTS 5      // Maximale Anzahl Reconnect-Versuche

// WebSocket-Konfiguration
#define WS_RECONNECT_INTERVAL 5000  // 5 Sekunden
#define WS_HEARTBEAT_INTERVAL 30000 // 30 Sekunden
#define WS_BUFFER_SIZE       4096   // WebSocket Buffer

// =============================================================================
// AUDIO-STREAMING-KONFIGURATION
// =============================================================================

#define AUDIO_CHUNK_SIZE    512     // Größe der Audio-Chunks
#define AUDIO_RING_BUFFER_SIZE 8192 // Ring-Puffer für Audio
#define AUDIO_SILENCE_THRESHOLD 100 // Schwellwert für Stille

// =============================================================================
// LED-KONFIGURATION
// =============================================================================

#define LED_NUM_PIXELS      1       // Eine LED
#define LED_BRIGHTNESS      50      // Helligkeit (0-255)
#define LED_UPDATE_INTERVAL 50      // Update-Intervall in ms

// LED-Farben für verschiedene Zustände
#define LED_COLOR_BOOT      0xFFFFFF    // Weiß
#define LED_COLOR_AP        0x0000FF    // Blau
#define LED_COLOR_WIFI      0x00FFFF    // Cyan
#define LED_COLOR_SERVER    0x00FF00    // Grün
#define LED_COLOR_CONNECTED 0x001000    // Dezentes Grün
#define LED_COLOR_ERROR     0xFF0000    // Rot
#define LED_COLOR_BUTTON    0xFFFFFF    // Weiß
#define LED_COLOR_LISTENING 0x00FFFF    // Cyan
#define LED_COLOR_PLAYING   0x00FF00    // Grün
#define LED_COLOR_OTA       0xFF00FF    // Violett

// =============================================================================
// ENERGIE-MANAGEMENT
// =============================================================================

#define DEEP_SLEEP_TIMEOUT  30000000 // 30 Sekunden in Mikrosekunden
#define BUTTON_WAKEUP_PIN   GPIO_NUM_39
#define BUTTON_WAKEUP_LEVEL 0        // Low-Level Wakeup

// =============================================================================
// NVS-KONFIGURATION
// =============================================================================

#define NVS_NAMESPACE       "m5echo"
#define NVS_KEY_WIFI_SSID   "wifi_ssid"
#define NVS_KEY_WIFI_PASS   "wifi_pass"
#define NVS_KEY_SERVER_HOST "server_host"
#define NVS_KEY_SERVER_PORT "server_port"
#define NVS_KEY_CLIENT_ID   "client_id"
#define NVS_KEY_MIC_MODE    "mic_mode"
#define NVS_KEY_STATIC_IP   "static_ip"
#define NVS_KEY_STATIC_GW   "static_gw"
#define NVS_KEY_STATIC_SM   "static_sm"

// =============================================================================
// WEBSOCKET-NACHRICHTEN-TYPEN
// =============================================================================

// Client → Server
#define WS_MSG_IDENTIFICATION "identification"
#define WS_MSG_EVENT         "event"
#define WS_MSG_AUDIO         "audio"

// Server → Client
#define WS_MSG_COMMAND       "command"
#define WS_MSG_CONFIG        "config"
#define WS_MSG_OTA           "ota"

// Event-Typen
#define EVENT_BUTTON_PRESSED "pressed"
#define EVENT_BUTTON_RELEASED "released"

// =============================================================================
// MANAGER-INTEGRATION & EVENTS
// =============================================================================

// Event-Typen für Cross-Manager-Kommunikation
enum class ManagerEventType {
    AUDIO_DATA_READY,      // AudioManager → WebSocketClient
    AUDIO_TEST,            // WifiManager → AudioManager
    LED_COMMAND,           // WebSocketClient → LedManager
    AUDIO_PLAY_COMMAND,    // WebSocketClient → AudioManager
    CONFIG_UPDATE,         // WebSocketClient → WifiManager
    OTA_COMMAND,           // WebSocketClient → OtaManager
    POWER_COMMAND,         // WebSocketClient → PowerManager
    BUTTON_PRESSED,        // PowerManager → alle Manager
    WIFI_STATUS_CHANGE,    // WifiManager → alle Manager
    ERROR_OCCURRED         // Alle Manager → alle Manager
};

// Event-Struktur für Manager-Kommunikation (vereinfacht für Stabilität)
struct ManagerEvent {
    ManagerEventType type;
    uint32_t sourceId;     // Manager-ID
    uint32_t timestamp;
    uint32_t dataSize;     // Größe der Daten
    uint8_t data[256];     // Feste Daten-Buffer (max 256 Bytes)
};

// Manager-IDs für Event-Quellen
#define MANAGER_ID_LED        0x01
#define MANAGER_ID_WIFI       0x02
#define MANAGER_ID_AUDIO      0x03
#define MANAGER_ID_WEBSOCKET  0x04
#define MANAGER_ID_POWER      0x05
#define MANAGER_ID_OTA        0x06

// Event-Queue-Konfiguration
#define MANAGER_EVENT_QUEUE_SIZE 20
#define MANAGER_EVENT_QUEUE_TIMEOUT 100

// Audio-Transfer-Konfiguration
#define AUDIO_TRANSFER_QUEUE_SIZE 10
#define AUDIO_CHUNK_TRANSFER_TIMEOUT 50

// =============================================================================
// FREERTOS TASK-PRIORITÄTEN
// =============================================================================

#define AUDIO_TASK_PRIORITY        5
#define WEBSOCKET_TASK_PRIORITY    4
#define LED_TASK_PRIORITY          3
#define WIFI_TASK_PRIORITY         2
#define POWER_TASK_PRIORITY        1
#define OTA_TASK_PRIORITY          2
#define EVENT_TASK_PRIORITY        3

// =============================================================================
// STACK-GRÖSSEN FÜR TASKS
// =============================================================================

#define AUDIO_TASK_STACK_SIZE      16384  // Reduziert da doppelte Task entfernt
#define WEBSOCKET_TASK_STACK_SIZE  40960
#define LED_TASK_STACK_SIZE        4096
#define WIFI_TASK_STACK_SIZE       4096
#define POWER_TASK_STACK_SIZE      4096
#define OTA_TASK_STACK_SIZE        8192
#define EVENT_TASK_STACK_SIZE      8192

// =============================================================================
// DEBUG-KONFIGURATION
// =============================================================================

#ifdef DEBUG
    #define DEBUG_PRINT(x) Serial.print(x)
    #define DEBUG_PRINTLN(x) Serial.println(x)
    #define DEBUG_PRINTF(fmt, ...) Serial.printf(fmt, ##__VA_ARGS__)
#else
    #define DEBUG_PRINT(x)
    #define DEBUG_PRINTLN(x)
    #define DEBUG_PRINTF(fmt, ...)
#endif

// =============================================================================
// STANDARDWERTE
// =============================================================================

#define DEFAULT_CLIENT_ID   "m5echo_001"
#define DEFAULT_UPDATE_SERVER_URL "http://192.168.1.100:8080/ota"
#define DEFAULT_SERVER_HOST "192.168.1.100"
#define DEFAULT_SERVER_PORT 8080
#define DEFAULT_MIC_MODE    "on_button_press"  // "always_on" oder "on_button_press"

#endif // CONFIG_H
