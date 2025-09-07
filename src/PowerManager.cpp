#include "PowerManager.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/timers.h>

// =============================================================================
// KONSTRUKTOR & DESTRUKTOR
// =============================================================================

PowerManager::PowerManager() {
    currentMode = PowerMode::ALWAYS_ON;
    currentState = PowerState::ACTIVE;
    lastWakeupSource = WakeupSource::UNKNOWN;
    
    // Sleep-Konfiguration
    sleepTimeout = DEEP_SLEEP_TIMEOUT;
    lastActivityTime = 0;
    sleepEnabled = true;
    
    // Wakeup-Konfiguration
    wakeupPin = BUTTON_WAKEUP_PIN;
    wakeupLevel = BUTTON_WAKEUP_LEVEL;
    wakeupCause = ESP_SLEEP_WAKEUP_UNDEFINED;
    
    // Activity-Tracking
    lastButtonPress = 0;
    lastAudioActivity = 0;
    lastNetworkActivity = 0;
    activityDetected = false;
    
    // Callbacks
    sleepCallback = nullptr;
    wakeupCallback = nullptr;
    
    // Sleep-Statistiken
    totalSleepTime = 0;
    lastSleepDuration = 0;
    wakeupCount = 0;
    sleepStartTime = 0;
    
    // Debug
    debugEnabled = false;
}

PowerManager::~PowerManager() {
    // Cleanup
}

// =============================================================================
// INITIALISIERUNG
// =============================================================================

void PowerManager::begin() {
    Serial.println("PowerManager: Initialisiere...");
    
    // Wakeup-Quellen konfigurieren
    configureWakeupSources();
    
    // Wakeup-Cause ermitteln
    handleWakeup();
    
    // Activity-Timer zurücksetzen
    resetActivityTimers();
    
    // Sleep-Modus konfigurieren
    configureSleepMode();
    
    currentState = PowerState::ACTIVE;
    Serial.println("PowerManager: Initialisierung abgeschlossen");
}

void PowerManager::update() {
    // Wakeup-Cause verarbeiten (nur beim ersten Update nach Wakeup)
    static bool wakeupProcessed = false;
    if (!wakeupProcessed && wakeupCause != ESP_SLEEP_WAKEUP_UNDEFINED) {
        handleWakeup();
        wakeupProcessed = true;
    }
    
    // Activity-Zeit aktualisieren
    updateActivityTime();
    
    // Sleep-Logik basierend auf Modus
    switch (currentMode) {
        case PowerMode::ALWAYS_ON:
            // Keine Sleep-Logik im Always-On-Modus
            break;
            
        case PowerMode::BUTTON_WAKE:
            // Sleep nach Timeout, wenn keine Aktivität
            if (shouldGoToSleep()) {
                goToSleep();
            }
            break;
            
        case PowerMode::TIMER_WAKE:
            // Timer-basierte Sleep-Logik
            if (shouldGoToSleep()) {
                goToSleep();
            }
            break;
            
        case PowerMode::DEEP_SLEEP:
            // Sofortiger Deep-Sleep
            if (currentState == PowerState::ACTIVE) {
                goToSleep();
            }
            break;
    }
}

// =============================================================================
// POWER-MODUS-STEUERUNG
// =============================================================================

void PowerManager::setPowerMode(PowerMode mode) {
    if (currentMode != mode) {
        Serial.printf("PowerManager: Wechsle Modus von %d zu %d\n", (int)currentMode, (int)mode);
        currentMode = mode;
        configureSleepMode();
    }
}

PowerMode PowerManager::getPowerMode() const {
    return currentMode;
}

PowerState PowerManager::getState() const {
    return currentState;
}

// =============================================================================
// SLEEP-STEUERUNG
// =============================================================================

void PowerManager::enableSleep(bool enabled) {
    sleepEnabled = enabled;
    Serial.printf("PowerManager: Sleep %s\n", enabled ? "aktiviert" : "deaktiviert");
}

void PowerManager::setSleepTimeout(unsigned long timeout) {
    sleepTimeout = timeout;
    Serial.printf("PowerManager: Sleep-Timeout auf %lu ms gesetzt\n", timeout);
}

void PowerManager::goToSleep() {
    if (!sleepEnabled || currentState == PowerState::SLEEPING) {
        return;
    }
    
    Serial.println("PowerManager: Gehe in Deep-Sleep...");
    currentState = PowerState::GOING_TO_SLEEP;
    
    // Sleep-Callback aufrufen
    if (sleepCallback) {
        sleepCallback();
    }
    
    // Sleep-Statistiken aktualisieren
    sleepStartTime = millis();
    
    // Kurze Verzögerung für Cleanup
    delay(100);
    
    // Deep-Sleep konfigurieren und starten
    esp_sleep_enable_ext0_wakeup(wakeupPin, wakeupLevel);
    
    // Zusätzliche Wakeup-Quellen je nach Modus
    switch (currentMode) {
        case PowerMode::BUTTON_WAKE:
            // Nur Button-Wakeup
            break;
            
        case PowerMode::TIMER_WAKE:
            // Timer + Button-Wakeup
            esp_sleep_enable_timer_wakeup(sleepTimeout * 1000); // Mikrosekunden
            break;
            
        case PowerMode::DEEP_SLEEP:
            // Nur Button-Wakeup
            break;
            
        default:
            break;
    }
    
    currentState = PowerState::SLEEPING;
    
    // Deep-Sleep starten
    esp_deep_sleep_start();
}

void PowerManager::wakeUp() {
    if (currentState == PowerState::SLEEPING) {
        Serial.println("PowerManager: Wakeup aus Deep-Sleep");
        currentState = PowerState::WAKING_UP;
        
        // Wakeup-Cause ermitteln
        wakeupCause = esp_sleep_get_wakeup_cause();
        
        // Activity-Timer zurücksetzen
        resetActivityTimers();
        
        currentState = PowerState::ACTIVE;
    }
}

bool PowerManager::isSleeping() const {
    return currentState == PowerState::SLEEPING;
}

// =============================================================================
// WAKEUP-KONFIGURATION
// =============================================================================

void PowerManager::setWakeupPin(gpio_num_t pin, int level) {
    wakeupPin = pin;
    wakeupLevel = level;
    configureWakeupSources();
}

void PowerManager::setWakeupTimer(unsigned long seconds) {
    esp_sleep_enable_timer_wakeup(seconds * 1000000); // Mikrosekunden
}

WakeupSource PowerManager::getLastWakeupSource() const {
    return lastWakeupSource;
}

esp_sleep_wakeup_cause_t PowerManager::getWakeupCause() const {
    return wakeupCause;
}

// =============================================================================
// ACTIVITY-TRACKING
// =============================================================================

void PowerManager::registerButtonActivity() {
    lastButtonPress = millis();
    lastActivityTime = lastButtonPress;
    activityDetected = true;
    
    if (debugEnabled) {
        Serial.println("PowerManager: Button-Aktivität registriert");
    }
}

void PowerManager::registerAudioActivity() {
    lastAudioActivity = millis();
    lastActivityTime = lastAudioActivity;
    activityDetected = true;
    
    if (debugEnabled) {
        Serial.println("PowerManager: Audio-Aktivität registriert");
    }
}

void PowerManager::registerNetworkActivity() {
    lastNetworkActivity = millis();
    lastActivityTime = lastNetworkActivity;
    activityDetected = true;
    
    if (debugEnabled) {
        Serial.println("PowerManager: Netzwerk-Aktivität registriert");
    }
}

unsigned long PowerManager::getLastActivityTime() const {
    return lastActivityTime;
}

bool PowerManager::hasRecentActivity() const {
    if (currentMode == PowerMode::ALWAYS_ON) {
        return true; // Immer aktiv im Always-On-Modus
    }
    
    unsigned long currentTime = millis();
    unsigned long timeSinceLastActivity = currentTime - lastActivityTime;
    
    // Aktivität in den letzten 5 Sekunden
    return timeSinceLastActivity < 5000;
}

// =============================================================================
// SLEEP-STATISTIKEN
// =============================================================================

unsigned long PowerManager::getTotalSleepTime() const {
    return totalSleepTime;
}

unsigned long PowerManager::getLastSleepDuration() const {
    return lastSleepDuration;
}

int PowerManager::getWakeupCount() const {
    return wakeupCount;
}

// =============================================================================
// UTILITY-METHODEN
// =============================================================================

void PowerManager::reset() {
    currentState = PowerState::ACTIVE;
    resetActivityTimers();
    wakeupCause = ESP_SLEEP_WAKEUP_UNDEFINED;
    lastWakeupSource = WakeupSource::UNKNOWN;
    
    Serial.println("PowerManager: Zurückgesetzt");
}

void PowerManager::forceWakeup() {
    if (currentState == PowerState::SLEEPING) {
        Serial.println("PowerManager: Erzwungenes Wakeup");
        wakeUp();
    }
}

void PowerManager::disableSleep() {
    sleepEnabled = false;
    Serial.println("PowerManager: Sleep deaktiviert");
}

bool PowerManager::isSleepEnabled() const {
    return sleepEnabled;
}

// =============================================================================
// DEBUG-METHODEN
// =============================================================================

void PowerManager::printPowerStatus() {
    Serial.printf("PowerManager: Modus=%d, Status=%d, Sleep=%s\n",
                  (int)currentMode,
                  (int)currentState,
                  sleepEnabled ? "aktiv" : "inaktiv");
    
    Serial.printf("PowerManager: Letzte Aktivität=%lu ms, Wakeup-Cause=%d\n",
                  lastActivityTime,
                  (int)wakeupCause);
}

void PowerManager::printSleepStats() {
    Serial.printf("PowerManager: Gesamte Sleep-Zeit=%lu ms, Letzte Sleep=%lu ms, Wakeups=%d\n",
                  totalSleepTime,
                  lastSleepDuration,
                  wakeupCount);
}

void PowerManager::enableDebug(bool enabled) {
    debugEnabled = enabled;
    Serial.printf("PowerManager: Debug %s\n", enabled ? "aktiviert" : "deaktiviert");
}

// =============================================================================
// CALLBACK-REGISTRIERUNG
// =============================================================================

void PowerManager::setSleepCallback(SleepCallback callback) {
    sleepCallback = callback;
}

void PowerManager::setWakeupCallback(WakeupCallback callback) {
    wakeupCallback = callback;
}

// =============================================================================
// EVENT-INTEGRATION
// =============================================================================

void PowerManager::processPowerCommand(const String& command) {
    Serial.printf("PowerManager: Verarbeite Power-Befehl: %s\n", command.c_str());
    
    // JSON-Befehl parsen
    DynamicJsonDocument doc(512);
    DeserializationError error = deserializeJson(doc, command);
    
    if (error) {
        Serial.printf("PowerManager: JSON-Parsing-Fehler: %s\n", error.c_str());
        return;
    }
    
    // Befehlstyp ermitteln
    const char* type = doc["type"];
    if (!type) {
        Serial.println("PowerManager: Kein Befehlstyp gefunden");
        return;
    }
    
    if (strcmp(type, "set_mode") == 0) {
        // Power-Modus setzen
        const char* mode = doc["mode"];
        if (mode) {
            if (strcmp(mode, "always_on") == 0) setPowerMode(PowerMode::ALWAYS_ON);
            else if (strcmp(mode, "button_wake") == 0) setPowerMode(PowerMode::BUTTON_WAKE);
            else if (strcmp(mode, "timer_wake") == 0) setPowerMode(PowerMode::TIMER_WAKE);
            else if (strcmp(mode, "deep_sleep") == 0) setPowerMode(PowerMode::DEEP_SLEEP);
        }
    }
    else if (strcmp(type, "set_timeout") == 0) {
        // Sleep-Timeout setzen
        if (doc.containsKey("timeout")) {
            unsigned long timeout = doc["timeout"];
            setSleepTimeout(timeout);
        }
    }
    else if (strcmp(type, "enable_sleep") == 0) {
        // Sleep aktivieren/deaktivieren
        if (doc.containsKey("enabled")) {
            bool enabled = doc["enabled"];
            enableSleep(enabled);
        }
    }
    else if (strcmp(type, "go_to_sleep") == 0) {
        // Sofortiger Deep-Sleep
        goToSleep();
    }
    else if (strcmp(type, "force_wakeup") == 0) {
        // Erzwungenes Wakeup
        forceWakeup();
    }
    else {
        Serial.printf("PowerManager: Unbekannter Befehlstyp: %s\n", type);
    }
}

// =============================================================================
// PRIVATE METHODEN
// =============================================================================

void PowerManager::configureWakeupSources() {
    // Button-Wakeup konfigurieren
    esp_sleep_enable_ext0_wakeup(wakeupPin, wakeupLevel);
    
    Serial.printf("PowerManager: Wakeup-Pin GPIO%d (Level=%d) konfiguriert\n",
                  wakeupPin,
                  wakeupLevel);
}

void PowerManager::configureSleepMode() {
    switch (currentMode) {
        case PowerMode::ALWAYS_ON:
            // Keine Sleep-Konfiguration
            Serial.println("PowerManager: Always-On-Modus - Keine Sleep-Konfiguration");
            break;
            
        case PowerMode::BUTTON_WAKE:
            // Nur Button-Wakeup
            esp_sleep_enable_ext0_wakeup(wakeupPin, wakeupLevel);
            Serial.println("PowerManager: Button-Wakeup-Modus konfiguriert");
            break;
            
        case PowerMode::TIMER_WAKE:
            // Timer + Button-Wakeup
            esp_sleep_enable_ext0_wakeup(wakeupPin, wakeupLevel);
            esp_sleep_enable_timer_wakeup(sleepTimeout * 1000);
            Serial.printf("PowerManager: Timer-Wakeup-Modus konfiguriert (Timeout=%lu ms)\n", sleepTimeout);
            break;
            
        case PowerMode::DEEP_SLEEP:
            // Nur Button-Wakeup
            esp_sleep_enable_ext0_wakeup(wakeupPin, wakeupLevel);
            Serial.println("PowerManager: Deep-Sleep-Modus konfiguriert");
            break;
    }
}

void PowerManager::handleWakeup() {
    wakeupCause = esp_sleep_get_wakeup_cause();
    wakeupCount++;
    
    // Sleep-Statistiken aktualisieren
    if (sleepStartTime > 0) {
        lastSleepDuration = millis() - sleepStartTime;
        totalSleepTime += lastSleepDuration;
        sleepStartTime = 0;
    }
    
    // Wakeup-Quelle ermitteln
    switch (wakeupCause) {
        case ESP_SLEEP_WAKEUP_EXT0:
            lastWakeupSource = WakeupSource::BUTTON;
            Serial.println("PowerManager: Wakeup durch Button");
            break;
            
        case ESP_SLEEP_WAKEUP_TIMER:
            lastWakeupSource = WakeupSource::TIMER;
            Serial.println("PowerManager: Wakeup durch Timer");
            break;
            
        case ESP_SLEEP_WAKEUP_EXT1:
            lastWakeupSource = WakeupSource::EXTERNAL_SOURCE;
            Serial.println("PowerManager: Wakeup durch externe Quelle");
            break;
            
        default:
            lastWakeupSource = WakeupSource::UNKNOWN;
            Serial.printf("PowerManager: Wakeup durch unbekannte Quelle (%d)\n", wakeupCause);
            break;
    }
    
    // Wakeup-Callback aufrufen
    if (wakeupCallback) {
        wakeupCallback(lastWakeupSource);
    }
    
    // Activity-Timer zurücksetzen
    resetActivityTimers();
}

void PowerManager::handleSleep() {
    Serial.println("PowerManager: Sleep-Handler aufgerufen");
    // Zusätzliche Sleep-Logik hier implementieren
}

bool PowerManager::shouldGoToSleep() {
    if (!sleepEnabled || currentMode == PowerMode::ALWAYS_ON) {
        return false;
    }
    
    unsigned long currentTime = millis();
    unsigned long timeSinceLastActivity = currentTime - lastActivityTime;
    
    // Sleep nach Timeout, wenn keine Aktivität
    if (timeSinceLastActivity > sleepTimeout) {
        if (debugEnabled) {
            Serial.printf("PowerManager: Sleep-Timeout erreicht (%lu ms seit letzter Aktivität)\n", timeSinceLastActivity);
        }
        return true;
    }
    
    return false;
}

void PowerManager::updateActivityTime() {
    // Activity-Zeit basierend auf neuester Aktivität aktualisieren
    unsigned long latestActivity = max(max(lastButtonPress, lastAudioActivity), lastNetworkActivity);
    
    if (latestActivity > lastActivityTime) {
        lastActivityTime = latestActivity;
    }
}

void PowerManager::resetActivityTimers() {
    unsigned long currentTime = millis();
    lastButtonPress = currentTime;
    lastAudioActivity = currentTime;
    lastNetworkActivity = currentTime;
    lastActivityTime = currentTime;
    activityDetected = false;
    
    if (debugEnabled) {
        Serial.println("PowerManager: Activity-Timer zurückgesetzt");
    }
}

// =============================================================================
// FEHLENDE MEMBER-VARIABLEN IN HEADER HINZUFÜGEN
// =============================================================================

// Diese Variablen müssen in der Header-Datei hinzugefügt werden:
// unsigned long totalSleepTime;
// unsigned long lastSleepDuration;
// int wakeupCount;
// unsigned long sleepStartTime;
// bool debugEnabled;
