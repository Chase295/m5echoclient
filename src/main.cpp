#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <Preferences.h>
#include <ESPmDNS.h>

#include "config.h"
#include "LedManager.h"
#include "WifiManager.h"
#include "AudioManager.h"
#include "WebSocketClient.h"
#include "PowerManager.h"
#include "OtaManager.h"
// #include "EventManager.h"  // Temporär deaktiviert

// =============================================================================
// LIVE-LOG-SYSTEM MIT SHORT POLLING
// =============================================================================

String logBuffer = "";
SemaphoreHandle_t logMutex;

// =============================================================================
// AUDIO-RICHTLINIEN
// =============================================================================

volatile bool micDebugModeActive = false;

// =============================================================================
// ROBUSTE TASTER-LOGIK
// =============================================================================

volatile bool buttonStateChanged = false;
volatile unsigned long lastInterruptTime = 0;
int lastButtonState = HIGH; // Wichtig: Annahme, dass der Knopf am Anfang nicht gedrückt ist

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
// LIVE-LOGGING FÜR WEB-CLIENTS
// =============================================================================

// Diese Funktion kann von überall im Code aufgerufen werden
void logToWebClients(const char* message) {
    Serial.println(message); // Parallel im seriellen Monitor ausgeben
    if (xSemaphoreTake(logMutex, portMAX_DELAY) == pdTRUE) {
        logBuffer += String(message) + "\n";
        xSemaphoreGive(logMutex);
    }
}

// =============================================================================
// AUDIO-RICHTLINIEN-IMPLEMENTIERUNG
// =============================================================================

void updateAudioState() {
    // Lese den aktuellen Betriebsmodus aus dem NVS
    Preferences prefs;
    prefs.begin("system-config", true); // read-only
    String currentMicMode = prefs.getString("mic_mode", "on_button_press");
    prefs.end();

    // Prüfe die Bedingungen
    bool shouldRecord = micDebugModeActive || 
                        (currentMicMode == "always_on" && webSocketClient.isConnected());

    if (shouldRecord && !audioManager.isRecording()) {
        logToWebClients("[System] Audio-Richtlinie: Aufnahme wird gestartet.");
        audioManager.startRecording();
    } else if (!shouldRecord && audioManager.isRecording()) {
        logToWebClients("[System] Audio-Richtlinie: Aufnahme wird gestoppt.");
        audioManager.stopRecording();
    }
}

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
    logToWebClients("Main: LED-Task gestartet");
    
    while (true) {
        ledManager.update();
        vTaskDelay(pdMS_TO_TICKS(LED_UPDATE_INTERVAL));
    }
}

void wifiTask(void* parameter) {
    logToWebClients("Main: WiFi-Task gestartet");
    
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
    buttonStateChanged = true;
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
    
    // ERST DAS SCHLOSS EINBAUEN:
    logMutex = xSemaphoreCreateMutex();
    if (logMutex == NULL) {
        Serial.println("FATAL: Konnte logMutex nicht erstellen!");
        // Hier evtl. in einer Endlosschleife anhalten
        while(1) {
            delay(1000);
            Serial.println("FATAL ERROR: logMutex konnte nicht erstellt werden!");
        }
    }
    
    // DANN ERST DEN REST INITIALISIEREN UND LOGGEN:
    logToWebClients("\n\n=== M5 Echo Client Firmware ===");
    logToWebClients("Version: 1.0.0");
    logToWebClients("Build: " __DATE__ " " __TIME__);
    
    // LED-Manager initialisieren
    ledManager.begin();
    ledManager.setState(LedState::BOOTING);
    logToWebClients("Main: LedManager initialisiert");
    
    // Power-Manager initialisieren
    powerManager.begin();
    logToWebClients("Main: PowerManager initialisiert");
    
    // WiFi-Manager initialisieren
    wifiManager.begin();
    logToWebClients("Main: WifiManager initialisiert");
    
    // Audio-Manager initialisieren
    if (audioManager.begin()) {
        logToWebClients("Main: AudioManager initialisiert");
        // Audio-Wiedergabe NICHT starten - das verursacht komische Geräusche
        // Audio-Wiedergabe nur starten wenn echte Audio-Daten verfügbar sind
        logToWebClients("Main: Audio-Wiedergabe bleibt gestoppt bis Audio-Daten empfangen werden");
    } else {
        logToWebClients("Main: AudioManager-Initialisierung fehlgeschlagen");
        ledManager.setState(LedState::ERROR);
    }
    
    // EventManager temporär deaktiviert
    // eventManager.begin();
    
    // WebSocket-Client initialisieren
    webSocketClient.begin();
    logToWebClients("Main: WebSocketClient initialisiert");
    
    // Live-Logging-System initialisiert
    logToWebClients("Main: Live-Logging-System initialisiert");
    
    // OTA-Manager initialisieren
    otaManager.begin();
    logToWebClients("Main: OtaManager initialisiert");
    
    // Manager-Integration temporär deaktiviert für WLAN-Stabilität
    // setupManagerIntegration();
    
    // Button-Interrupt einrichten
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonISR, CHANGE); // Wichtig: Auf CHANGE ändern
    logToWebClients("Main: Robuster Button-Interrupt für Drücken & Loslassen eingerichtet.");
    
    // FreeRTOS-Tasks erstellen
    createTasks();
    
    // WiFi-Verbindung versuchen
    currentAppState = AppState::CONNECTING;
    ledManager.setState(LedState::NO_WIFI);
    
    // Warte kurz für stabile Initialisierung
    delay(2000);
    
    if (wifiManager.connect()) {
        logToWebClients("Main: WiFi-Verbindung erfolgreich");
        currentAppState = AppState::CONNECTED;
        ledManager.setState(LedState::IDLE);
        
        // Webserver im Client-Modus starten
        logToWebClients("Main: Starte Webserver im Client-Modus...");
        wifiManager.startWebServer();
        
        // WebSocket-Verbindung aufbauen
        if (webSocketClient.connect(DEFAULT_SERVER_HOST, DEFAULT_SERVER_PORT)) {
            logToWebClients("Main: WebSocket-Verbindung erfolgreich");
            ledManager.setState(LedState::IDLE);
            
            // Audio-Richtlinie aktualisieren
            updateAudioState();
        } else {
            logToWebClients("Main: WebSocket-Verbindung fehlgeschlagen");
            ledManager.setState(LedState::NO_WEBSOCKET);
            
            // Audio-Richtlinie aktualisieren
            updateAudioState();
        }
    } else {
        logToWebClients("Main: WiFi-Verbindung fehlgeschlagen - Starte AP-Modus");
        currentAppState = AppState::CONFIGURING;
        ledManager.setState(LedState::NO_WIFI);
        wifiManager.startAccessPointMode();
    }
    
    logToWebClients("Main: Setup abgeschlossen");
    
    // Initiale Audio-Richtlinie setzen
    updateAudioState();
    
    // --- FINALER MIKROFON-TEST WIRD ERZWUNGEN ---
    logToWebClients("--- FINALER MIKROFON-TEST WIRD ERZWUNGEN ---");
    micDebugModeActive = true;
    updateAudioState(); // Stellt sicher, dass die Aufnahme-Task gestartet wird
}

void loop() {
    // --- HERZSCHLAG-LOG ZUR DIAGNOSE ---
    static unsigned long lastHeartbeat = 0;
    if (millis() - lastHeartbeat > 5000) { // Alle 5 Sekunden
        lastHeartbeat = millis();
        logToWebClients("[Herzschlag] Main Loop lebt...");
    }
    // --- ENDE HERZSCHLAG-LOG ---
    
    // Haupt-Loop ist minimal, da alles in FreeRTOS-Tasks läuft
    
    // --- NEUE, ROBUSTE TASTER-LOGIK ---
    if (buttonStateChanged) {
        buttonStateChanged = false; // Flag sofort zurücksetzen

        // Kurze Entprell-Verzögerung
        vTaskDelay(pdMS_TO_TICKS(50));

        int currentButtonState = digitalRead(BUTTON_PIN);

        if (currentButtonState != lastButtonState) {
            if (currentButtonState == LOW) {
                // Der Knopf wurde gerade gedrückt (Wechsel von HIGH zu LOW)
                logToWebClients("Knopf gedrückt.");
                // Hier weitere Aktionen für den Knopfdruck...
            } else {
                // Der Knopf wurde gerade losgelassen (Wechsel von LOW zu HIGH)
                logToWebClients("Knopf losgelassen.");
                // Hier weitere Aktionen für das Loslassen...
            }
            lastButtonState = currentButtonState; // Neuen Zustand merken
        }
    }
    // --- ENDE TASTER-LOGIK ---
    
    // *** WEBSERVER-ANFRAGEN BEARBEITEN ***
    // Bearbeite eingehende Webserver-Anfragen in jedem Schleifendurchlauf
    wifiManager.handleClient();
    

    
    // Webserver-Handle für Client-Modus (DNS-Server für AP-Modus)
    if (wifiManager.isConnected() && !wifiManager.isAccessPointMode()) {
        wifiManager.handleWebServer();
    }
    
    // mDNS-Updates
    // MDNS.update(); // Nicht benötigt, da mDNS automatisch aktualisiert wird
    
    // Status-Updates und LED-Steuerung
    static unsigned long lastStatusUpdate = 0;
    if (millis() - lastStatusUpdate > 5000) { // Alle 5 Sekunden
        lastStatusUpdate = millis();
        
        // LED-Status basierend auf aktueller Situation setzen
        if (wifiManager.isConnected()) {
            if (webSocketClient.isConnected()) {
                ledManager.setState(LedState::IDLE);
                currentAppState = AppState::CONNECTED;
            } else {
                ledManager.setState(LedState::NO_WEBSOCKET);
                currentAppState = AppState::CONNECTING;
            }
        } else if (wifiManager.isAccessPointMode()) {
            ledManager.setState(LedState::NO_WIFI);
            currentAppState = AppState::CONFIGURING;
        } else {
            ledManager.setState(LedState::NO_WIFI);
            currentAppState = AppState::CONNECTING;
        }
        
        // Audio-Richtlinie bei Status-Änderungen aktualisieren
        updateAudioState();
        
        Serial.printf("Main: Status - App: %d, WiFi: %s, WebSocket: %s, Audio: %s\n",
                      (int)currentAppState,
                      wifiManager.isConnected() ? "verbunden" : "getrennt",
                      webSocketClient.isConnected() ? "verbunden" : "getrennt",
                      audioManager.isRecording() ? "aktiv" : "inaktiv");
    }
    
    // *** MIKROFON-DEBUG-AUSGABE ***
    static unsigned long lastMicDebugTime = 0;
    if (micDebugModeActive && millis() - lastMicDebugTime > 1000) {
        lastMicDebugTime = millis();
        uint16_t level = audioManager.getCurrentAudioLevel();
        char logMsg[50];
        sprintf(logMsg, "[Mic-Debug] Aktuelle Lautstärke: %u", level);
        logToWebClients(logMsg);
    }
    
    // Kurze Verzögerung
    delay(100);
}
