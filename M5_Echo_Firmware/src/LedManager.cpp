#include "LedManager.h"
#include <FastLED.h>

// =============================================================================
// KONSTRUKTOR & DESTRUKTOR
// =============================================================================

LedManager::LedManager() {
    pixels = nullptr;
    currentState = LedState::BOOTING;
    lastUpdate = 0;
    breathingDirection = true;
    breathingBrightness = LED_BRIGHTNESS;
    
    // Animation-Timer für nicht-blockierende Effekte
    lastBreathingUpdate = 0;
    lastPulseUpdate = 0;
    lastBlinkUpdate = 0;
    pulseState = false;
    blinkState = false;
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
    
    // Zustandsspezifische Updates
    switch (currentState) {
        case LedState::BOOTING:
            // Kurz aufleuchten beim Booten
            setColor(LED_COLOR_BOOT);
            break;
            
        case LedState::ACCESS_POINT:
            // Pulsieren im Access-Point-Modus
            setPulsingEffect(LED_COLOR_AP);
            break;
            
        case LedState::WIFI_CONNECTING:
            // Blinken beim WLAN-Verbindungsaufbau
            setBlinkingEffect(LED_COLOR_WIFI);
            break;
            
        case LedState::SERVER_CONNECTING:
            // Blinken beim Server-Verbindungsaufbau
            setBlinkingEffect(LED_COLOR_SERVER);
            break;
            
        case LedState::CONNECTED:
            // Dezentes Grün oder aus
            setColor(0x001000); // Sehr dezentes Grün
            break;
            
        case LedState::ERROR:
            // Rot bei Fehlern
            setColor(LED_COLOR_ERROR);
            break;
            
        case LedState::BUTTON_PRESSED:
            // Weiß bei Tastendruck
            setColor(LED_COLOR_BUTTON);
            break;
            
        case LedState::LISTENING:
            // Atmen beim Zuhören
            setBreathingEffect(LED_COLOR_LISTENING);
            break;
            
        case LedState::PLAYING:
            // Pulsieren bei Audio-Wiedergabe
            setPulsingEffect(LED_COLOR_PLAYING);
            break;
            
        case LedState::OTA_UPDATE:
            // Violett bei OTA-Update
            setPulsingEffect(LED_COLOR_OTA);
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

void LedManager::setBrightness(uint8_t brightness) {
    FastLED.setBrightness(brightness);
    FastLED.show();
}

void LedManager::turnOff() {
    FastLED.clear();
    FastLED.show();
}

void LedManager::turnOn() {
    setColor(LED_COLOR_CONNECTED);
}

// =============================================================================
// SPEZIELLE EFFEKTE
// =============================================================================

void LedManager::startBreathing(uint32_t color) {
    setBreathingEffect(color);
}

void LedManager::startPulsing(uint32_t color) {
    setPulsingEffect(color);
}

void LedManager::startBlinking(uint32_t color) {
    setBlinkingEffect(color);
}

void LedManager::stopEffect() {
    // Effekt stoppen und zur normalen Farbe zurückkehren
    setColor(getColorForState(currentState));
}

// =============================================================================
// UTILITY-METHODEN
// =============================================================================

bool LedManager::isStateActive(LedState state) const {
    return currentState == state;
}

void LedManager::forceUpdate() {
    lastUpdate = 0; // Sofortige Aktualisierung erzwingen
    update();
}

// =============================================================================
// PRIVATE METHODEN
// =============================================================================

void LedManager::setBreathingEffect(uint32_t color) {
    // Breathing-Effekt implementieren (nicht-blockierend)
    unsigned long currentTime = millis();
    
    if (currentTime - lastBreathingUpdate > 50) { // 50ms für sanftes Atmen
        lastBreathingUpdate = currentTime;
        
        // Helligkeit ändern
        if (breathingDirection) {
            breathingBrightness += 5;
            if (breathingBrightness >= LED_BRIGHTNESS) {
                breathingBrightness = LED_BRIGHTNESS;
                breathingDirection = false;
            }
        } else {
            breathingBrightness -= 5;
            if (breathingBrightness <= 10) {
                breathingBrightness = 10;
                breathingDirection = true;
            }
        }
        
        // Farbe mit aktueller Helligkeit setzen
        uint8_t r = (color >> 16) & 0xFF;
        uint8_t g = (color >> 8) & 0xFF;
        uint8_t b = color & 0xFF;
        
        r = (r * breathingBrightness) / LED_BRIGHTNESS;
        g = (g * breathingBrightness) / LED_BRIGHTNESS;
        b = (b * breathingBrightness) / LED_BRIGHTNESS;
        
        setColor(r, g, b);
    }
}

void LedManager::setPulsingEffect(uint32_t color) {
    // Pulsieren-Effekt implementieren (nicht-blockierend)
    unsigned long currentTime = millis();
    
    if (currentTime - lastPulseUpdate > 500) { // 500ms für Pulsieren
        lastPulseUpdate = currentTime;
        pulseState = !pulseState;
        
        if (pulseState) {
            setColor(color);
        } else {
            setColor(0x000000); // Aus
        }
    }
}

void LedManager::setBlinkingEffect(uint32_t color) {
    // Blinken-Effekt implementieren (nicht-blockierend)
    unsigned long currentTime = millis();
    
    if (currentTime - lastBlinkUpdate > 200) { // 200ms für Blinken
        lastBlinkUpdate = currentTime;
        blinkState = !blinkState;
        
        if (blinkState) {
            setColor(color);
        } else {
            setColor(0x000000); // Aus
        }
    }
}

uint32_t LedManager::getColorForState(LedState state) {
    switch (state) {
        case LedState::BOOTING: return LED_COLOR_BOOT;
        case LedState::ACCESS_POINT: return LED_COLOR_AP;
        case LedState::WIFI_CONNECTING: return LED_COLOR_WIFI;
        case LedState::SERVER_CONNECTING: return LED_COLOR_SERVER;
        case LedState::CONNECTED: return 0x001000; // Dezentes Grün
        case LedState::ERROR: return LED_COLOR_ERROR;
        case LedState::BUTTON_PRESSED: return LED_COLOR_BUTTON;
        case LedState::LISTENING: return LED_COLOR_LISTENING;
        case LedState::PLAYING: return LED_COLOR_PLAYING;
        case LedState::OTA_UPDATE: return LED_COLOR_OTA;
        default: return 0x000000; // Aus
    }
}

void LedManager::updateBreathingEffect() {
    // Diese Methode wird von setBreathingEffect verwendet
    // Implementierung ist bereits in setBreathingEffect enthalten
}

// =============================================================================
// EVENT-INTEGRATION
// =============================================================================

void LedManager::processCommand(const String& command) {
    Serial.printf("LedManager: Verarbeite Befehl: %s\n", command.c_str());
    
    // JSON-Befehl parsen
    DynamicJsonDocument doc(512);
    DeserializationError error = deserializeJson(doc, command);
    
    if (error) {
        Serial.printf("LedManager: JSON-Parsing-Fehler: %s\n", error.c_str());
        return;
    }
    
    // Befehlstyp ermitteln
    const char* type = doc["type"];
    if (!type) {
        Serial.println("LedManager: Kein Befehlstyp gefunden");
        return;
    }
    
    if (strcmp(type, "set_state") == 0) {
        // LED-Zustand setzen
        const char* state = doc["state"];
        if (state) {
            if (strcmp(state, "booting") == 0) setState(LedState::BOOTING);
            else if (strcmp(state, "connected") == 0) setState(LedState::CONNECTED);
            else if (strcmp(state, "error") == 0) setState(LedState::ERROR);
            else if (strcmp(state, "listening") == 0) setState(LedState::LISTENING);
            else if (strcmp(state, "playing") == 0) setState(LedState::PLAYING);
            else if (strcmp(state, "ota_update") == 0) setState(LedState::OTA_UPDATE);
            else if (strcmp(state, "idle") == 0) setState(LedState::IDLE);
        }
    }
    else if (strcmp(type, "set_color") == 0) {
        // LED-Farbe setzen
        if (doc.containsKey("r") && doc.containsKey("g") && doc.containsKey("b")) {
            uint8_t r = doc["r"];
            uint8_t g = doc["g"];
            uint8_t b = doc["b"];
            setColor(r, g, b);
        }
    }
    else if (strcmp(type, "set_brightness") == 0) {
        // LED-Helligkeit setzen
        if (doc.containsKey("brightness")) {
            uint8_t brightness = doc["brightness"];
            setBrightness(brightness);
        }
    }
    else if (strcmp(type, "start_effect") == 0) {
        // LED-Effekt starten
        const char* effect = doc["effect"];
        if (effect) {
            if (strcmp(effect, "breathing") == 0) {
                uint32_t color = doc["color"] | 0xFFFFFF;
                startBreathing(color);
            }
            else if (strcmp(effect, "pulsing") == 0) {
                uint32_t color = doc["color"] | 0xFFFFFF;
                startPulsing(color);
            }
            else if (strcmp(effect, "blinking") == 0) {
                uint32_t color = doc["color"] | 0xFFFFFF;
                startBlinking(color);
            }
        }
    }
    else if (strcmp(type, "stop_effect") == 0) {
        // LED-Effekt stoppen
        stopEffect();
    }
    else {
        Serial.printf("LedManager: Unbekannter Befehlstyp: %s\n", type);
    }
}
