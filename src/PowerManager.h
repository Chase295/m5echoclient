#ifndef POWER_MANAGER_H
#define POWER_MANAGER_H

#include <Arduino.h>
#include <esp_sleep.h>
#include <driver/rtc_io.h>
#include <ArduinoJson.h>
#include "config.h"

// Energie-Modi
enum class PowerMode {
    ALWAYS_ON,      // Dauerbetrieb
    BUTTON_WAKE,    // Taster-Wakeup
    TIMER_WAKE,     // Timer-Wakeup
    DEEP_SLEEP      // Deep-Sleep
};

// Wakeup-Quellen
enum class WakeupSource {
    BUTTON,         // Taster
    TIMER,          // Timer
    EXTERNAL_SOURCE, // Externe Quelle
    UNKNOWN         // Unbekannt
};

// Power-Management-Zustand
enum class PowerState {
    ACTIVE,         // Aktiv
    GOING_TO_SLEEP, // Geht in Schlaf
    SLEEPING,       // Schlafend
    WAKING_UP,      // Wacht auf
    ERROR           // Fehler
};

class PowerManager {
private:
    PowerMode currentMode;
    PowerState currentState;
    WakeupSource lastWakeupSource;
    
    // Sleep-Konfiguration
    unsigned long sleepTimeout;
    unsigned long lastActivityTime;
    bool sleepEnabled;
    
    // Wakeup-Konfiguration
    gpio_num_t wakeupPin;
    int wakeupLevel;
    esp_sleep_wakeup_cause_t wakeupCause;
    
    // Activity-Tracking
    unsigned long lastButtonPress;
    unsigned long lastAudioActivity;
    unsigned long lastNetworkActivity;
    bool activityDetected;
    
    // Sleep-Statistiken
    unsigned long totalSleepTime;
    unsigned long lastSleepDuration;
    int wakeupCount;
    unsigned long sleepStartTime;
    
    // Debug
    bool debugEnabled;
    
    // Private Methoden
    void configureWakeupSources();
    void configureSleepMode();
    void handleWakeup();
    void handleSleep();
    void updateActivityTime();
    void resetActivityTimers();
    
    // Öffentliche Methoden für EventManager

public:
    // Konstruktor & Destruktor
    PowerManager();
    ~PowerManager();
    
    // Initialisierung
    void begin();
    void update();
    
    // Power-Modus-Steuerung
    void setPowerMode(PowerMode mode);
    PowerMode getPowerMode() const;
    PowerState getState() const;
    
    // Sleep-Steuerung
    void enableSleep(bool enabled);
    void setSleepTimeout(unsigned long timeout);
    void goToSleep();
    void wakeUp();
    bool isSleeping() const;
    
    // Wakeup-Konfiguration
    void setWakeupPin(gpio_num_t pin, int level);
    void setWakeupTimer(unsigned long seconds);
    WakeupSource getLastWakeupSource() const;
    esp_sleep_wakeup_cause_t getWakeupCause() const;
    
    // Activity-Tracking
    void registerButtonActivity();
    void registerAudioActivity();
    void registerNetworkActivity();
    unsigned long getLastActivityTime() const;
    bool hasRecentActivity() const;
    
    // Sleep-Statistiken
    unsigned long getTotalSleepTime() const;
    unsigned long getLastSleepDuration() const;
    int getWakeupCount() const;
    
    // Utility-Methoden
    void reset();
    void forceWakeup();
    void disableSleep();
    bool isSleepEnabled() const;
    bool shouldGoToSleep();
    
    // Debug-Methoden
    void printPowerStatus();
    void printSleepStats();
    void enableDebug(bool enabled);
    
    // Callback-Funktionen
    typedef void (*SleepCallback)();
    typedef void (*WakeupCallback)(WakeupSource source);
    
    void setSleepCallback(SleepCallback callback);
    void setWakeupCallback(WakeupCallback callback);
    
    // Event-Integration
    void processPowerCommand(const String& command);
    
private:
    SleepCallback sleepCallback;
    WakeupCallback wakeupCallback;
};

#endif // POWER_MANAGER_H
