#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <Preferences.h>
#include "config.h"
#include "LedManager.h"
#include "WifiManager.h"
#include "AudioManager.h"
#include "WebSocketClient.h"
#include "PowerManager.h"
#include "OtaManager.h"
// #include "EventManager.h"  // Temporär deaktiviert

// =============================================================================
// GLOBALE MANAGER-INSTANZEN
// =============================================================================

LedManager ledManager;
WifiManager wifiManager;
AudioManager audioManager;
WebSocketClient webSocketClient;
PowerManager powerManager;
OtaManager otaManager;
// EventManager eventManager;  // Temporär deaktiviert

// =============================================================================
// FREERTOS-TASKS
// =============================================================================

TaskHandle_t ledTaskHandle = nullptr;
TaskHandle_t wifiTaskHandle = nullptr;
// audioTaskHandle entfernt - AudioManager verwaltet seine eigenen Tasks
TaskHandle_t websocketTaskHandle = nullptr;
TaskHandle_t powerTaskHandle = nullptr;
TaskHandle_t otaTaskHandle = nullptr;

// =============================================================================
// ANWENDUNGSZUSTAND
// =============================================================================

enum class AppState {
    BOOTING,
    CONFIGURING,
    CONNECTING,
    CONNECTED,
    ERROR,
    SLEEPING
};

AppState currentAppState = AppState::BOOTING;

// =============================================================================
// TASK-FUNKTIONEN
// =============================================================================

void ledTask(void* parameter) {
    Serial.println("Main: LED-Task gestartet");
    
    while (true) {
        ledManager.update();
        vTaskDelay(pdMS_TO_TICKS(LED_UPDATE_INTERVAL));
    }
}

void wifiTask(void* parameter) {
    Serial.println("Main: WiFi-Task gestartet");
    
    while (true) {
        wifiManager.update();
        
        // WiFi-Status an EventManager melden (temporär deaktiviert)
        // if (wifiManager.isConnected()) {
        //     eventManager.sendWifiStatusChange("connected");
        // } else {
        //     eventManager.sendWifiStatusChange("disconnected");
        // }
        
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// Audio-Task entfernt - AudioManager verwaltet seine eigenen Tasks

void websocketTask(void* parameter) {
    Serial.println("Main: WebSocket-Task gestartet");
    
    while (true) {
        webSocketClient.update();
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void powerTask(void* parameter) {
    Serial.println("Main: Power-Task gestartet");
    
    while (true) {
        powerManager.update();
        
        // Power-Management-Logik
        if (powerManager.shouldGoToSleep()) {
            Serial.println("Main: Gehe in Deep-Sleep...");
            currentAppState = AppState::SLEEPING;
            powerManager.goToSleep();
        }
        
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void otaTask(void* parameter) {
    Serial.println("Main: OTA-Task gestartet");
    
    while (true) {
        otaManager.update();
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

// =============================================================================
// INTERRUPT-HANDLER
// =============================================================================

void IRAM_ATTR buttonISR() {
    // Button-Press an EventManager senden
    // eventManager.sendButtonPressed();  // Temporär deaktiviert
}

// =============================================================================
// MANAGER-INTEGRATION
// =============================================================================

void setupManagerIntegration() {
    Serial.println("Main: Setup Manager-Integration...");
    
    // EventManager temporär deaktiviert für WLAN-Stabilität
    // eventManager.begin();
    
    // Manager beim EventManager registrieren
    // eventManager.registerLedManager(&ledManager);
    // eventManager.registerWifiManager(&wifiManager);
    // eventManager.registerAudioManager(&audioManager);
    // eventManager.registerWebSocketClient(&webSocketClient);
    // eventManager.registerPowerManager(&powerManager);
    // eventManager.registerOtaManager(&otaManager);
    
    // EventManager bei Managern setzen
    // audioManager.setEventManager(&eventManager);
    // wifiManager.setEventManager(&eventManager);
    // otaManager.setLedManager(&ledManager);
    
    Serial.println("Main: Manager-Integration abgeschlossen");
}

void createTasks() {
    Serial.println("Main: Erstelle FreeRTOS-Tasks...");
    
    // LED-Task
    xTaskCreate(
        ledTask,
        "led_task",
        LED_TASK_STACK_SIZE,
        nullptr,
        LED_TASK_PRIORITY,
        &ledTaskHandle
    );
    
    // WiFi-Task
    xTaskCreate(
        wifiTask,
        "wifi_task",
        WIFI_TASK_STACK_SIZE,
        nullptr,
        WIFI_TASK_PRIORITY,
        &wifiTaskHandle
    );
    
    // Audio-Task entfernt - AudioManager verwaltet seine eigenen Tasks
    
    // WebSocket-Task
    xTaskCreate(
        websocketTask,
        "websocket_task",
        WEBSOCKET_TASK_STACK_SIZE,
        nullptr,
        WEBSOCKET_TASK_PRIORITY,
        &websocketTaskHandle
    );
    
    // Power-Task
    xTaskCreate(
        powerTask,
        "power_task",
        POWER_TASK_STACK_SIZE,
        nullptr,
        POWER_TASK_PRIORITY,
        &powerTaskHandle
    );
    
    // OTA-Task
    xTaskCreate(
        otaTask,
        "ota_task",
        OTA_TASK_STACK_SIZE,
        nullptr,
        OTA_TASK_PRIORITY,
        &otaTaskHandle
    );
    
    Serial.println("Main: FreeRTOS-Tasks erstellt");
}

// =============================================================================
// ARDUINO SETUP & LOOP
// =============================================================================

void setup() {
    // Serial-Kommunikation initialisieren
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n\n=== M5 Echo Client Firmware ===");
    Serial.println("Version: 1.0.0");
    Serial.println("Build: " __DATE__ " " __TIME__);
    
    // LED-Manager initialisieren
    ledManager.begin();
    ledManager.setState(LedState::BOOTING);
    Serial.println("Main: LedManager initialisiert");
    
    // Power-Manager initialisieren
    powerManager.begin();
    Serial.println("Main: PowerManager initialisiert");
    
    // WiFi-Manager initialisieren
    wifiManager.begin();
    Serial.println("Main: WifiManager initialisiert");
    
    // Audio-Manager initialisieren
    if (audioManager.begin()) {
        Serial.println("Main: AudioManager initialisiert");
        // Audio-Wiedergabe NICHT starten - das verursacht komische Geräusche
        // Audio-Wiedergabe nur starten wenn echte Audio-Daten verfügbar sind
        Serial.println("Main: Audio-Wiedergabe bleibt gestoppt bis Audio-Daten empfangen werden");
    } else {
        Serial.println("Main: AudioManager-Initialisierung fehlgeschlagen");
        ledManager.setState(LedState::ERROR);
    }
    
    // EventManager temporär deaktiviert
    // eventManager.begin();
    
    // WebSocket-Client initialisieren
    webSocketClient.begin();
    Serial.println("Main: WebSocketClient initialisiert");
    
    // OTA-Manager initialisieren
    otaManager.begin();
    Serial.println("Main: OtaManager initialisiert");
    
    // Manager-Integration temporär deaktiviert für WLAN-Stabilität
    // setupManagerIntegration();
    
    // Button-Interrupt einrichten
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonISR, FALLING);
    Serial.println("Main: Button-Interrupt eingerichtet");
    
    // FreeRTOS-Tasks erstellen
    createTasks();
    
    // WiFi-Verbindung versuchen
    currentAppState = AppState::CONNECTING;
    ledManager.setState(LedState::WIFI_CONNECTING);
    
    // Warte kurz für stabile Initialisierung
    delay(2000);
    
    if (wifiManager.connect()) {
        Serial.println("Main: WiFi-Verbindung erfolgreich");
        currentAppState = AppState::CONNECTED;
        ledManager.setState(LedState::CONNECTED);
        
        // WebSocket-Verbindung aufbauen
        if (webSocketClient.connect(DEFAULT_SERVER_HOST, DEFAULT_SERVER_PORT)) {
            Serial.println("Main: WebSocket-Verbindung erfolgreich");
            ledManager.setState(LedState::CONNECTED);
        } else {
            Serial.println("Main: WebSocket-Verbindung fehlgeschlagen");
            ledManager.setState(LedState::ERROR);
        }
    } else {
        Serial.println("Main: WiFi-Verbindung fehlgeschlagen - Starte AP-Modus");
        currentAppState = AppState::CONFIGURING;
        ledManager.setState(LedState::ACCESS_POINT);
        wifiManager.startAccessPointMode();
    }
    
    Serial.println("Main: Setup abgeschlossen");
}

void loop() {
    // Haupt-Loop ist minimal, da alles in FreeRTOS-Tasks läuft
    
    // Status-Updates und LED-Steuerung
    static unsigned long lastStatusUpdate = 0;
    if (millis() - lastStatusUpdate > 5000) { // Alle 5 Sekunden
        lastStatusUpdate = millis();
        
        // LED-Status basierend auf aktueller Situation setzen
        if (wifiManager.isConnected()) {
            if (webSocketClient.isConnected()) {
                ledManager.setState(LedState::CONNECTED);
                currentAppState = AppState::CONNECTED;
            } else {
                ledManager.setState(LedState::WIFI_CONNECTING);
                currentAppState = AppState::CONNECTING;
            }
        } else if (wifiManager.isAccessPointMode()) {
            ledManager.setState(LedState::ACCESS_POINT);
            currentAppState = AppState::CONFIGURING;
        } else {
            ledManager.setState(LedState::WIFI_CONNECTING);
            currentAppState = AppState::CONNECTING;
        }
        
        Serial.printf("Main: Status - App: %d, WiFi: %s, WebSocket: %s, Audio: %s\n",
                      (int)currentAppState,
                      wifiManager.isConnected() ? "verbunden" : "getrennt",
                      webSocketClient.isConnected() ? "verbunden" : "getrennt",
                      audioManager.isRecording() ? "aktiv" : "inaktiv");
    }
    
    // Kurze Verzögerung
    delay(100);
}
