#ifndef LED_MANAGER_H
#define LED_MANAGER_H

#include <Arduino.h>
#include <FastLED.h>
#include <ArduinoJson.h>
#include "config.h"

// LED-Zustände entsprechend der Spezifikation
enum class LedState {
    BOOTING,
    NO_WIFI,
    NO_WEBSOCKET,
    LISTENING,
    PLAYING,
    IDLE,
    THINKING,
    OTA_UPDATE, // <-- HIER HINZUFÜGEN
    ERROR // Behalten wir als allgemeinen Fehlerzustand
};

// Anzahl der LED-Zustände
const int TOTAL_LED_STATES = 9;

// Enum für die verfügbaren Effekte
enum class LedEffect { 
    STATIC, 
    PULSING, 
    BREATHING 
};

// Struct, das alle Einstellungen für EINEN Zustand enthält
struct LedStateConfig {
    bool active = true;
    uint32_t color = 0x000000;
    uint8_t brightness = 128;
    LedEffect effect = LedEffect::STATIC;
    uint8_t speed = 50; // Effekt-Geschwindigkeit (z.B. 1-100)
    
    // --- NEUE PARAMETER ---
    uint8_t minBrightness = 0;   // Für Atmen-Effekt
    uint8_t maxBrightness = 255; // Für Atmen-Effekt
    bool rainbowMode = false;      // Für Regenbogen-Atmen-Effekt
};

class LedManager {
private:
    CRGB* pixels;
    LedState currentState;
    unsigned long lastUpdate;
    bool breathingDirection;
    int breathingBrightness;
    
    // Ein Array, das die Konfiguration für jeden LedState speichert
    LedStateConfig m_stateConfigs[TOTAL_LED_STATES];
    

    
    // Animation-Timer für nicht-blockierende Effekte
    unsigned long lastBreathingUpdate;
    unsigned long lastPulseUpdate;
    unsigned long lastBlinkUpdate;
    bool pulseState;
    bool blinkState;
    
    // Regenbogen-Effekt
    uint8_t g_hue;
    
    // Private Methoden
    void setBreathingEffect(uint32_t color, uint8_t minBrightness, uint8_t maxBrightness, bool rainbowMode);
    void setPulsingEffect(uint32_t color);
    void setBlinkingEffect(uint32_t color);
    void updateBreathingEffect();

public:
    // Konstruktor & Destruktor
    LedManager();
    ~LedManager();
    
    // Initialisierung
    void begin();
    void update();
    
    // Zustandssteuerung
    void setState(LedState state);
    LedState getState() const;
    
    // Direkte LED-Steuerung
    void setColor(uint8_t r, uint8_t g, uint8_t b);
    void setColor(uint32_t color);
    void setColor(CHSV hsv);
    void setBrightness(uint8_t brightness);
    void turnOff();
    void turnOn();
    
    // Spezielle Effekte
    void startBreathing(uint32_t color);
    void startPulsing(uint32_t color);
    void startBlinking(uint32_t color);
    void stopEffect();
    
    // Utility-Methoden
    bool isStateActive(LedState state) const;
    void forceUpdate();
    
    // Konfiguration
    void loadConfig();
    void saveConfig();
    LedStateConfig* getStateConfig(LedState state);
    String getStateName(LedState state);
    void initializeDefaultConfigs();
    
    // Manueller Override-Modus

    
    // Event-Integration
    void processCommand(const String& command);
};

#endif // LED_MANAGER_H
