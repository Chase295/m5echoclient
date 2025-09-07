#include "LedManager.h"
#include <FastLED.h>
#include <Preferences.h>
#include <ArduinoJson.h>

// =============================================================================
// KONSTRUKTOR & DESTRUKTOR
// =============================================================================

LedManager::LedManager() {
    pixels = nullptr;
    currentState = LedState::BOOTING;
    lastUpdate = 0;
    breathingDirection = true;
    breathingBrightness = LED_BRIGHTNESS;
    

    
    // Standard-Konfiguration für alle Zustände initialisieren
    initializeDefaultConfigs();
    
    // Animation-Timer für nicht-blockierende Effekte
    lastBreathingUpdate = 0;
    lastPulseUpdate = 0;
    lastBlinkUpdate = 0;
    pulseState = false;
    blinkState = false;
    
    // Regenbogen-Effekt
    g_hue = 0;
}

LedManager::~LedManager() {
    if (pixels) {
        delete[] pixels;
        pixels = nullptr;
    }
}

// =============================================================================
// INITIALISIERUNG
// =============================================================================

void LedManager::begin() {
    // FastLED-Konfiguration
    pixels = new CRGB[LED_NUM_PIXELS];
    FastLED.addLeds<WS2812, LED_PIN, GRB>(pixels, LED_NUM_PIXELS);
    FastLED.setBrightness(LED_BRIGHTNESS);
    FastLED.clear();
    FastLED.show();
    
    // Konfiguration laden
    loadConfig();
    
    // Initialen Zustand setzen
    setState(LedState::BOOTING);
}

void LedManager::update() {
    unsigned long currentTime = millis();
    
    // Nur alle LED_UPDATE_INTERVAL ms aktualisieren
    if (currentTime - lastUpdate < LED_UPDATE_INTERVAL) {
        return;
    }
    
    lastUpdate = currentTime;
    
    // Konfiguration für aktuellen Zustand abrufen
    LedStateConfig* config = getStateConfig(currentState);
    if (!config || !config->active) {
        // LED ausschalten wenn Zustand inaktiv oder nicht konfiguriert
        turnOff();
        return;
    }
    
    // Helligkeit setzen
    FastLED.setBrightness(config->brightness);
    
    // Effekt basierend auf Konfiguration anwenden
    switch (config->effect) {
        case LedEffect::STATIC:
            setColor(config->color);
            break;
            
        case LedEffect::PULSING:
            setPulsingEffect(config->color);
            break;
            
        case LedEffect::BREATHING:
            setBreathingEffect(config->color, config->minBrightness, config->maxBrightness, config->rainbowMode);
            break;
            
        default:
            setColor(config->color);
            break;
    }
}

// =============================================================================
// ZUSTANDSSTEUERUNG
// =============================================================================

void LedManager::setState(LedState state) {
    if (currentState != state) {
        currentState = state;
        forceUpdate();
    }
}

LedState LedManager::getState() const {
    return currentState;
}

// =============================================================================
// DIREKTE LED-STEUERUNG
// =============================================================================

void LedManager::setColor(uint8_t r, uint8_t g, uint8_t b) {
    pixels[0] = CRGB(r, g, b);
    FastLED.show();
}

void LedManager::setColor(uint32_t color) {
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;
    setColor(r, g, b);
}

void LedManager::setColor(CHSV hsv) {
    CRGB rgb;
    hsv2rgb_rainbow(hsv, rgb);
    setColor(rgb.r, rgb.g, rgb.b);
}

void LedManager::setBrightness(uint8_t brightness) {
    FastLED.setBrightness(brightness);
    FastLED.show();
}

void LedManager::turnOff() {
    FastLED.clear();
    FastLED.show();
}

void LedManager::turnOn() {
    LedStateConfig* config = getStateConfig(currentState);
    if (config && config->active) {
        setColor(config->color);
    }
}

// =============================================================================
// SPEZIELLE EFFEKTE
// =============================================================================

void LedManager::startBreathing(uint32_t color) {
    LedStateConfig* config = getStateConfig(currentState);
    if (config) {
        setBreathingEffect(color, config->minBrightness, config->maxBrightness, config->rainbowMode);
    } else {
        setBreathingEffect(color, 0, 255, false); // Fallback mit Standardwerten
    }
}

void LedManager::startPulsing(uint32_t color) {
    setPulsingEffect(color);
}

void LedManager::startBlinking(uint32_t color) {
    setBlinkingEffect(color);
}

void LedManager::stopEffect() {
    // Effekt stoppen und zur normalen Farbe zurückkehren
    LedStateConfig* config = getStateConfig(currentState);
    if (config && config->active) {
        setColor(config->color);
    }
}

// =============================================================================
// UTILITY-METHODEN
// =============================================================================

bool LedManager::isStateActive(LedState state) const {
    return currentState == state;
}

void LedManager::forceUpdate() {
    lastUpdate = 0; // Sofortige Aktualisierung erzwingen
}

// =============================================================================
// EFFEKT-IMPLEMENTIERUNGEN
// =============================================================================

void LedManager::setBreathingEffect(uint32_t color, uint8_t minBrightness, uint8_t maxBrightness, bool rainbowMode) {
    unsigned long currentTime = millis();
    
    if (currentTime - lastBreathingUpdate > 50) { // 50ms für sanfte Animation
        if (breathingDirection) {
            breathingBrightness += 5;
            if (breathingBrightness >= maxBrightness) {
                breathingBrightness = maxBrightness;
                breathingDirection = false;
            }
        } else {
            breathingBrightness -= 5;
            if (breathingBrightness <= minBrightness) {
                breathingBrightness = minBrightness;
                breathingDirection = true;
            }
        }
        
        if (rainbowMode) {
            // Regenbogen-Modus: Farbe durch HSV wandern lassen
            g_hue++; // Hue langsam erhöhen
            uint8_t currentBrightness = map(breathingBrightness, minBrightness, maxBrightness, 0, 255);
            setColor(CHSV(g_hue, 255, currentBrightness));
        } else {
            // Normaler Modus: Farbe mit Helligkeit modulieren
            uint8_t r = ((color >> 16) & 0xFF) * breathingBrightness / 255;
            uint8_t g = ((color >> 8) & 0xFF) * breathingBrightness / 255;
            uint8_t b = (color & 0xFF) * breathingBrightness / 255;
            
            setColor(r, g, b);
        }
        
        lastBreathingUpdate = currentTime;
    }
}

void LedManager::setPulsingEffect(uint32_t color) {
    unsigned long currentTime = millis();
    
    if (currentTime - lastPulseUpdate > 500) { // 500ms für Pulsieren
        pulseState = !pulseState;
        
        if (pulseState) {
            setColor(color);
        } else {
            turnOff();
        }
        
        lastPulseUpdate = currentTime;
    }
}

void LedManager::setBlinkingEffect(uint32_t color) {
    unsigned long currentTime = millis();
    
    if (currentTime - lastBlinkUpdate > 200) { // 200ms für schnelles Blinken
        blinkState = !blinkState;
        
        if (blinkState) {
            setColor(color);
        } else {
            turnOff();
        }
        
        lastBlinkUpdate = currentTime;
    }
}

// =============================================================================
// KONFIGURATION
// =============================================================================

void LedManager::initializeDefaultConfigs() {
    // BOOTING
    m_stateConfigs[0] = {true, 0xFFFFFF, 128, LedEffect::STATIC, 50, 0, 255, false}; // Weiß, statisch
    
    // NO_WIFI
    m_stateConfigs[1] = {true, 0x00FFFF, 128, LedEffect::PULSING, 50, 0, 255, false}; // Cyan, pulsierend
    
    // NO_WEBSOCKET
    m_stateConfigs[2] = {true, 0x00FFFF, 128, LedEffect::BREATHING, 50, 0, 255, false}; // Cyan, atmend
    
    // LISTENING
    m_stateConfigs[3] = {true, 0x00FFFF, 128, LedEffect::BREATHING, 50, 0, 255, false}; // Cyan, atmend
    
    // PLAYING
    m_stateConfigs[4] = {true, 0x0000FF, 128, LedEffect::PULSING, 50, 0, 255, false}; // Blau, pulsierend
    
    // IDLE
    m_stateConfigs[5] = {true, 0x001000, 128, LedEffect::STATIC, 50, 0, 255, false}; // Dezentes Grün
    
    // THINKING
    m_stateConfigs[6] = {true, 0x800080, 128, LedEffect::PULSING, 50, 0, 255, false}; // Violett, pulsierend
    
    // OTA_UPDATE
    m_stateConfigs[7] = {true, 0xFF8000, 128, LedEffect::PULSING, 50, 0, 255, false}; // Orange, pulsierend
    
    // ERROR
    m_stateConfigs[8] = {true, 0xFF0000, 128, LedEffect::PULSING, 50, 0, 255, false}; // Rot, pulsierend
    
    Serial.println("[LedManager] Standard-Konfigurationen initialisiert");
}

void LedManager::loadConfig() {
    Preferences prefs;
    prefs.begin("led-config", true); // read-only
    
    String configJson = prefs.getString("config", "");
    prefs.end();
    
    if (configJson.length() > 0) {
        // JSON parsen und Konfiguration laden
        DynamicJsonDocument doc(2048);
        DeserializationError error = deserializeJson(doc, configJson);
        
        if (error) {
            Serial.printf("[LedManager] JSON-Parsing-Fehler: %s\n", error.c_str());
            Serial.println("[LedManager] Verwende Standard-Konfigurationen");
            return;
        }
        
        // Konfigurationen aus JSON laden
        for (int i = 0; i < TOTAL_LED_STATES; i++) {
            String stateName = getStateName((LedState)i);
            if (doc.containsKey(stateName)) {
                JsonObject stateObj = doc[stateName];
                
                m_stateConfigs[i].active = stateObj["active"] | true;
                m_stateConfigs[i].color = strtol(stateObj["color"].as<String>().c_str(), NULL, 16);
                m_stateConfigs[i].brightness = stateObj["brightness"] | 128;
                
                String effectStr = stateObj["effect"] | "STATIC";
                if (effectStr == "PULSING") m_stateConfigs[i].effect = LedEffect::PULSING;
                else if (effectStr == "BREATHING") m_stateConfigs[i].effect = LedEffect::BREATHING;
                else m_stateConfigs[i].effect = LedEffect::STATIC;
                
                m_stateConfigs[i].speed = stateObj["speed"] | 50;
                
                // Neue Parameter laden
                m_stateConfigs[i].minBrightness = stateObj["minBrightness"] | 0;
                m_stateConfigs[i].maxBrightness = stateObj["maxBrightness"] | 255;
                m_stateConfigs[i].rainbowMode = stateObj["rainbowMode"] | false;
            }
        }
        
        Serial.println("[LedManager] Konfiguration aus NVS erfolgreich geladen");
    } else {
        Serial.println("[LedManager] Keine gespeicherte Konfiguration gefunden, verwende Standardwerte");
    }
}

void LedManager::saveConfig() {
    Preferences prefs;
    prefs.begin("led-config", false);
    
    // JSON-Dokument erstellen
    DynamicJsonDocument doc(2048);
    
    // Alle Zustände in JSON schreiben
    for (int i = 0; i < TOTAL_LED_STATES; i++) {
        String stateName = getStateName((LedState)i);
        JsonObject stateObj = doc.createNestedObject(stateName);
        
        stateObj["active"] = m_stateConfigs[i].active;
        
        char colorHex[7];
        sprintf(colorHex, "%06X", m_stateConfigs[i].color);
        stateObj["color"] = String(colorHex);
        
        stateObj["brightness"] = m_stateConfigs[i].brightness;
        
        String effectStr;
        switch (m_stateConfigs[i].effect) {
            case LedEffect::PULSING: effectStr = "PULSING"; break;
            case LedEffect::BREATHING: effectStr = "BREATHING"; break;
            default: effectStr = "STATIC"; break;
        }
        stateObj["effect"] = effectStr;
        
        stateObj["speed"] = m_stateConfigs[i].speed;
        
        // Neue Parameter speichern
        stateObj["minBrightness"] = m_stateConfigs[i].minBrightness;
        stateObj["maxBrightness"] = m_stateConfigs[i].maxBrightness;
        stateObj["rainbowMode"] = m_stateConfigs[i].rainbowMode;
    }
    
    String configJson;
    serializeJson(doc, configJson);
    
    prefs.putString("config", configJson);
    prefs.end();
    
    Serial.println("[LedManager] Konfiguration erfolgreich im NVS gespeichert");
}

LedStateConfig* LedManager::getStateConfig(LedState state) {
    int index = (int)state;
    if (index >= 0 && index < TOTAL_LED_STATES) {
        return &m_stateConfigs[index];
    }
    return nullptr;
}

String LedManager::getStateName(LedState state) {
    switch (state) {
        case LedState::BOOTING: return "BOOTING";
        case LedState::NO_WIFI: return "NO_WIFI";
        case LedState::NO_WEBSOCKET: return "NO_WEBSOCKET";
        case LedState::LISTENING: return "LISTENING";
        case LedState::PLAYING: return "PLAYING";
        case LedState::IDLE: return "IDLE";
        case LedState::THINKING: return "THINKING";
        case LedState::OTA_UPDATE: return "OTA_UPDATE";
        case LedState::ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}


