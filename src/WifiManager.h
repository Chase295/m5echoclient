#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include "config.h"

// Forward-Deklaration (temporär deaktiviert)
// class EventManager;

// WLAN-Verbindungsstatus
enum class WifiStatus {
    DISCONNECTED,    // Nicht verbunden
    CONNECTING,      // Verbindungsaufbau läuft
    CONNECTED,       // Verbunden
    ACCESS_POINT,    // Access-Point-Modus aktiv
    ERROR            // Fehler aufgetreten
};

// Netzwerk-Konfiguration
struct NetworkConfig {
    String ssid;
    String password;
    String staticIP;
    String staticGateway;
    String staticSubnet;
    bool useStaticIP;
};

class WifiManager {
private:
    WebServer* webServer;
    DNSServer* dnsServer;
    Preferences* preferences;
    
    WifiStatus currentStatus;
    NetworkConfig config;
    unsigned long lastReconnectAttempt;
    int reconnectAttempts;
    bool serverInitialized;  // Neue Member-Variable für Webserver-Status
    // EventManager* eventManager;  // Temporär deaktiviert
    
    // Private Methoden
    void startAccessPoint();
    void stopAccessPoint();
    void setupWebServer();
    void handleRoot();
    void handleConfig();
    void handleSave();
    void handleScan();
    void handleTestAudio();
    void handleReboot();
    void handleSaveLedConfig();
    void handleGetLedConfig();
    void handleGetSystemConfig();
    void handleSetMicMode();
    void handleSetMicDebug();
    void handleSSE();
    void handleGetLogs();
    void handleStatusJson();
    void handleNotFound();
    bool loadConfigFromNVS();
    bool saveConfigToNVS();
    void handleDNSServer();

public:
    // Konstruktor & Destruktor
    WifiManager();
    ~WifiManager();
    
    // Initialisierung
    void begin();
    void update();
    
    // WLAN-Verbindung
    bool connect(const String& ssid, const String& password);
    bool connect();
    void disconnect();
    bool isConnected() const;
    WifiStatus getStatus() const;
    
    // Access-Point-Modus
    void startAccessPointMode();
    void stopAccessPointMode();
    bool isAccessPointMode() const;
    
    // Konfiguration
    void setConfig(const NetworkConfig& config);
    NetworkConfig getConfig() const;
    bool loadConfig();
    bool saveConfig();
    
    // Netzwerk-Informationen
    String getLocalIP() const;
    String getSSID() const;
    int getRSSI() const;
    String getMacAddress() const;
    
    // Utility-Methoden
    void resetConfig();
    void forceReconnect();
    void setStaticIP(const String& ip, const String& gateway, const String& subnet);
    void useDHCP();
    
    // Web-Server-Kontrolle
    void startWebServer();  // Neue public-Methode
    void handleClient();    // Neue public-Methode für Webserver-Anfragen
    void handleWebServer();
    bool isWebServerActive() const;
    
    // Event-Integration
    void processConfigUpdate(const String& config);
    // void setEventManager(EventManager* manager);  // Temporär deaktiviert
};

#endif // WIFI_MANAGER_H
