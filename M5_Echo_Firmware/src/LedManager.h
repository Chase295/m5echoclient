#ifndef LED_MANAGER_H
#define LED_MANAGER_H

#include <Arduino.h>
#include <FastLED.h>
#include <ArduinoJson.h>
#include "config.h"

// LED-Zustände entsprechend der Spezifikation
enum class LedState {
    BOOTING,            // Booten / Startvorgang
    ACCESS_POINT,       // Access-Point-Modus
    WIFI_CONNECTING,    // WLAN-Verbindungsaufbau
    SERVER_CONNECTING,  // Server-Verbindungsaufbau
    CONNECTED,          // Verbunden & Bereit
    ERROR,              // Fehler (Kein WLAN/Server)
    BUTTON_PRESSED,     // Taster gedrückt
    LISTENING,          // Mikrofon aktiv / Hört zu
    PLAYING,            // Audio-Wiedergabe
    OTA_UPDATE,         // OTA Update läuft
    INSTALLING,         // Installation läuft
    SUCCESS,            // Erfolg
    IDLE                // Ruhezustand
};

class LedManager {
private:
    CRGB* pixels;
    LedState currentState;
    unsigned long lastUpdate;
    bool breathingDirection;
    int breathingBrightness;
    
    // Animation-Timer für nicht-blockierende Effekte
    unsigned long lastBreathingUpdate;
    unsigned long lastPulseUpdate;
    unsigned long lastBlinkUpdate;
    bool pulseState;
    bool blinkState;
    
    // Private Methoden
    void setColor(uint32_t color);
    void setBreathingEffect(uint32_t color);
    void setPulsingEffect(uint32_t color);
    void setBlinkingEffect(uint32_t color);
    uint32_t getColorForState(LedState state);
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
    
    // Event-Integration
    void processCommand(const String& command);
};

#endif // LED_MANAGER_H
