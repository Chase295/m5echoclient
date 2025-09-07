#include "WifiManager.h"
// #include "EventManager.h"  // Temporär deaktiviert

// =============================================================================
// KONSTRUKTOR & DESTRUKTOR
// =============================================================================

WifiManager::WifiManager() {
    webServer = nullptr;
    dnsServer = nullptr;
    preferences = nullptr;
    
    currentStatus = WifiStatus::DISCONNECTED;
    lastReconnectAttempt = 0;
    reconnectAttempts = 0;
    
    // Standard-Konfiguration
    config.ssid = "";
    config.password = "";
    config.staticIP = "";
    config.staticGateway = "";
    config.staticSubnet = "";
    config.useStaticIP = false;
}

WifiManager::~WifiManager() {
    if (webServer) {
        delete webServer;
        webServer = nullptr;
    }
    if (dnsServer) {
        delete dnsServer;
        dnsServer = nullptr;
    }
    if (preferences) {
        preferences->end();
        delete preferences;
        preferences = nullptr;
    }
}

// =============================================================================
// INITIALISIERUNG
// =============================================================================

void WifiManager::begin() {
    // Preferences initialisieren
    preferences = new Preferences();
    preferences->begin(NVS_NAMESPACE, false);
    
    // Konfiguration aus NVS laden
    loadConfigFromNVS();
    
    // WiFi-Modus auf Station setzen
    WiFi.mode(WIFI_STA);
    
    // DNS-Server für Captive Portal vorbereiten
    dnsServer = new DNSServer();
    
    Serial.println("WifiManager: Initialisiert");
}

void WifiManager::update() {
    // Web-Server verarbeiten (falls aktiv)
    if (webServer && isAccessPointMode()) {
        webServer->handleClient();
        dnsServer->processNextRequest();
    }
    
    // Automatische Wiederverbindung
    if (currentStatus == WifiStatus::DISCONNECTED && 
        !isAccessPointMode() && 
        config.ssid.length() > 0) {
        
        unsigned long currentTime = millis();
        if (currentTime - lastReconnectAttempt > WIFI_RECONNECT_INTERVAL) {
            if (reconnectAttempts < WIFI_MAX_RECONNECT_ATTEMPTS) {
                Serial.println("WifiManager: Automatische Wiederverbindung...");
                connect();
                reconnectAttempts++;
            } else {
                Serial.println("WifiManager: Maximale Reconnect-Versuche erreicht, starte AP-Modus");
                startAccessPointMode();
            }
            lastReconnectAttempt = currentTime;
        }
    }
}

// =============================================================================
// WLAN-VERBINDUNG
// =============================================================================

bool WifiManager::connect(const String& ssid, const String& password) {
    Serial.printf("WifiManager: Verbinde mit %s...\n", ssid.c_str());
    
    currentStatus = WifiStatus::CONNECTING;
    
    // Statische IP konfigurieren (falls gesetzt)
    if (config.useStaticIP && config.staticIP.length() > 0) {
        IPAddress ip, gateway, subnet;
        if (ip.fromString(config.staticIP) && 
            gateway.fromString(config.staticGateway) && 
            subnet.fromString(config.staticSubnet)) {
            WiFi.config(ip, gateway, subnet);
        }
    }
    
    // Verbindung versuchen
    WiFi.begin(ssid.c_str(), password.c_str());
    
    // Auf Verbindung warten
    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED && 
           (millis() - startTime) < WIFI_TIMEOUT_MS) {
        delay(500);
        Serial.print(".");
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        currentStatus = WifiStatus::CONNECTED;
        reconnectAttempts = 0;
        Serial.printf("\nWifiManager: Verbunden! IP: %s\n", WiFi.localIP().toString().c_str());
        return true;
    } else {
        currentStatus = WifiStatus::ERROR;
        Serial.println("\nWifiManager: Verbindung fehlgeschlagen!");
        return false;
    }
}

bool WifiManager::connect() {
    if (config.ssid.length() == 0) {
        Serial.println("WifiManager: Keine SSID konfiguriert - Starte AP-Modus");
        startAccessPointMode();
        return false;
    }
    
    Serial.printf("WifiManager: Verbinde mit %s...\n", config.ssid.c_str());
    bool success = connect(config.ssid, config.password);
    
    if (!success) {
        Serial.println("WifiManager: Verbindung fehlgeschlagen - Starte AP-Modus");
        startAccessPointMode();
    }
    
    return success;
}

void WifiManager::disconnect() {
    WiFi.disconnect();
    currentStatus = WifiStatus::DISCONNECTED;
    Serial.println("WifiManager: Verbindung getrennt");
}

bool WifiManager::isConnected() const {
    return WiFi.status() == WL_CONNECTED;
}

WifiStatus WifiManager::getStatus() const {
    if (isAccessPointMode()) {
        return WifiStatus::ACCESS_POINT;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        return WifiStatus::CONNECTED;
    }
    
    return currentStatus;
}

// =============================================================================
// ACCESS-POINT-MODUS
// =============================================================================

void WifiManager::startAccessPointMode() {
    Serial.println("WifiManager: Starte Access-Point-Modus");
    
    // WiFi-Modus auf AP+STA setzen
    WiFi.mode(WIFI_AP_STA);
    
    // Access Point konfigurieren
    IPAddress apIP;
    apIP.fromString(AP_IP);
    
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
    WiFi.softAP(AP_SSID, AP_PASSWORD);
    
    // Web-Server starten
    startAccessPoint();
    
    currentStatus = WifiStatus::ACCESS_POINT;
    Serial.printf("WifiManager: AP gestartet - SSID: %s, IP: %s\n", AP_SSID, AP_IP);
}

void WifiManager::stopAccessPointMode() {
    if (webServer) {
        webServer->close();
    }
    if (dnsServer) {
        dnsServer->stop();
    }
    
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_STA);
    
    currentStatus = WifiStatus::DISCONNECTED;
    Serial.println("WifiManager: Access-Point-Modus beendet");
}

bool WifiManager::isAccessPointMode() const {
    return WiFi.softAPgetStationNum() > 0 || currentStatus == WifiStatus::ACCESS_POINT;
}

// =============================================================================
// KONFIGURATION
// =============================================================================

void WifiManager::setConfig(const NetworkConfig& newConfig) {
    config = newConfig;
    saveConfigToNVS();
}

NetworkConfig WifiManager::getConfig() const {
    return config;
}

bool WifiManager::loadConfig() {
    return loadConfigFromNVS();
}

bool WifiManager::saveConfig() {
    return saveConfigToNVS();
}

// =============================================================================
// NETZWERK-INFORMATIONEN
// =============================================================================

String WifiManager::getLocalIP() const {
    return WiFi.localIP().toString();
}

String WifiManager::getSSID() const {
    return WiFi.SSID();
}

int WifiManager::getRSSI() const {
    return WiFi.RSSI();
}

String WifiManager::getMacAddress() const {
    return WiFi.macAddress();
}

// =============================================================================
// UTILITY-METHODEN
// =============================================================================

void WifiManager::resetConfig() {
    preferences->clear();
    config.ssid = "";
    config.password = "";
    config.staticIP = "";
    config.staticGateway = "";
    config.staticSubnet = "";
    config.useStaticIP = false;
    Serial.println("WifiManager: Konfiguration zurückgesetzt");
}

void WifiManager::forceReconnect() {
    disconnect();
    reconnectAttempts = 0;
    lastReconnectAttempt = 0;
    connect();
}

void WifiManager::setStaticIP(const String& ip, const String& gateway, const String& subnet) {
    config.staticIP = ip;
    config.staticGateway = gateway;
    config.staticSubnet = subnet;
    config.useStaticIP = true;
    saveConfigToNVS();
}

void WifiManager::useDHCP() {
    config.useStaticIP = false;
    saveConfigToNVS();
}

// =============================================================================
// WEB-SERVER-KONTROLLE
// =============================================================================

void WifiManager::handleWebServer() {
    if (webServer && isAccessPointMode()) {
        webServer->handleClient();
        dnsServer->processNextRequest();
    }
}

bool WifiManager::isWebServerActive() const {
    return webServer != nullptr && isAccessPointMode();
}

// =============================================================================
// PRIVATE METHODEN
// =============================================================================

void WifiManager::startAccessPoint() {
    if (webServer) {
        delete webServer;
    }
    
    webServer = new WebServer(80);
    setupWebServer();
    webServer->begin();
    
    // DNS-Server für Captive Portal
    dnsServer->start(53, "*", IPAddress(192, 168, 4, 1));
    
    Serial.println("WifiManager: Web-Server gestartet");
}

void WifiManager::stopAccessPoint() {
    if (webServer) {
        webServer->close();
    }
    if (dnsServer) {
        dnsServer->stop();
    }
}

void WifiManager::setupWebServer() {
    // Hauptseite (Konfigurationsformular)
    webServer->on("/", HTTP_GET, [this]() { handleRoot(); });
    
    // Konfiguration speichern
    webServer->on("/save", HTTP_POST, [this]() { handleSave(); });
    
    // WLAN-Scan
    webServer->on("/scan", HTTP_GET, [this]() { handleScan(); });
    
    // Audio-Test
    webServer->on("/test-audio", HTTP_POST, [this]() { handleTestAudio(); });
    
    // 404-Handler
    webServer->onNotFound([this]() { handleNotFound(); });
}

void WifiManager::handleRoot() {
    String html = R"(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>M5 Echo - WLAN-Konfiguration</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; background: #f0f0f0; }
        .container { max-width: 400px; margin: 0 auto; background: white; padding: 20px; border-radius: 8px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }
        h1 { color: #333; text-align: center; margin-bottom: 30px; }
        .form-group { margin-bottom: 15px; }
        label { display: block; margin-bottom: 5px; font-weight: bold; color: #555; }
        input[type="text"], input[type="password"] { width: 100%; padding: 10px; border: 1px solid #ddd; border-radius: 4px; box-sizing: border-box; }
        button { width: 100%; padding: 12px; background: #007bff; color: white; border: none; border-radius: 4px; cursor: pointer; font-size: 16px; }
        button:hover { background: #0056b3; }
        .status { margin-top: 20px; padding: 10px; border-radius: 4px; }
        .success { background: #d4edda; color: #155724; border: 1px solid #c3e6cb; }
        .error { background: #f8d7da; color: #721c24; border: 1px solid #f5c6cb; }
    </style>
</head>
<body>
    <div class="container">
        <h1>M5 Echo Setup</h1>
        <form id="wifiForm" action="/save" method="post">
            <div class="form-group">
                <label for="ssid">WLAN-Name (SSID):</label>
                <input type="text" id="ssid" name="ssid" required>
            </div>
            <div class="form-group">
                <label for="password">WLAN-Passwort:</label>
                <input type="password" id="password" name="password" required>
            </div>
            <button type="submit">Speichern & Verbinden</button>
        </form>
        <div id="status"></div>
        
        <div style=\"margin-top: 20px; padding: 15px; background: #f8f9fa; border-radius: 4px;\">
            <h3>Audio Test</h3>
            <button onclick=\"testAudio()\" style=\"background: #28a745; margin-top: 10px;\">Test-Ton abspielen</button>
        </div>
    </div>
    
    <script>
        function testAudio() {
            fetch('/test-audio', {method: 'POST'})
                .then(response => response.text())
                .then(data => {
                    const status = document.getElementById('status');
                    status.className = 'status success';
                    status.textContent = 'Test-Ton gespielt: ' + data;
                })
                .catch(error => {
                    const status = document.getElementById('status');
                    status.className = 'status error';
                    status.textContent = 'Fehler beim Test-Ton: ' + error;
                });
        }
        // Automatischer WLAN-Scan beim Laden
        window.onload = function() {
            fetch('/scan')
                .then(response => response.json())
                .then(data => {
                    if (data.networks && data.networks.length > 0) {
                        const ssidInput = document.getElementById('ssid');
                        ssidInput.value = data.networks[0].ssid;
                    }
                })
                .catch(error => console.log('Scan fehlgeschlagen:', error));
        };
        
        // Formular-Handler
        document.getElementById('wifiForm').onsubmit = function(e) {
            e.preventDefault();
            
            const formData = new FormData(this);
            const data = {
                ssid: formData.get('ssid'),
                password: formData.get('password')
            };
            
            fetch('/save', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify(data)
            })
            .then(response => response.json())
            .then(result => {
                const status = document.getElementById('status');
                if (result.success) {
                    status.className = 'status success';
                    status.textContent = 'Konfiguration gespeichert! Gerät startet neu...';
                    setTimeout(() => {
                        status.textContent = 'Bitte warten Sie, während das Gerät neu startet...';
                    }, 2000);
                } else {
                    status.className = 'status error';
                    status.textContent = 'Fehler: ' + result.message;
                }
            })
            .catch(error => {
                const status = document.getElementById('status');
                status.className = 'status error';
                status.textContent = 'Fehler beim Speichern der Konfiguration';
            });
        };
    </script>
</body>
</html>
    )";
    
    webServer->send(200, "text/html", html);
}

void WifiManager::handleSave() {
    if (webServer->hasArg("plain")) {
        // JSON-Daten verarbeiten
        String jsonData = webServer->arg("plain");
        
        // Einfache JSON-Parsing (für bessere Robustheit könnte ArduinoJson verwendet werden)
        int ssidStart = jsonData.indexOf("\"ssid\":\"") + 8;
        int ssidEnd = jsonData.indexOf("\"", ssidStart);
        int passStart = jsonData.indexOf("\"password\":\"") + 12;
        int passEnd = jsonData.indexOf("\"", passStart);
        
        if (ssidStart > 7 && ssidEnd > ssidStart && passStart > 11 && passEnd > passStart) {
            String ssid = jsonData.substring(ssidStart, ssidEnd);
            String password = jsonData.substring(passStart, passEnd);
            
            // Konfiguration speichern
            config.ssid = ssid;
            config.password = password;
            
            if (saveConfigToNVS()) {
                webServer->send(200, "application/json", "{\"success\":true,\"message\":\"Konfiguration gespeichert\"}");
                
                // Kurze Verzögerung für die Antwort
                delay(1000);
                
                // Gerät neu starten
                ESP.restart();
            } else {
                webServer->send(500, "application/json", "{\"success\":false,\"message\":\"Fehler beim Speichern\"}");
            }
        } else {
            webServer->send(400, "application/json", "{\"success\":false,\"message\":\"Ungültige Daten\"}");
        }
    } else {
        // Fallback für Form-Daten
        String ssid = webServer->hasArg("ssid") ? webServer->arg("ssid") : "";
        String password = webServer->hasArg("password") ? webServer->arg("password") : "";
        
        if (ssid.length() > 0 && password.length() > 0) {
            config.ssid = ssid;
            config.password = password;
            
            if (saveConfigToNVS()) {
                webServer->send(200, "application/json", "{\"success\":true,\"message\":\"Konfiguration gespeichert\"}");
                delay(1000);
                ESP.restart();
            } else {
                webServer->send(500, "application/json", "{\"success\":false,\"message\":\"Fehler beim Speichern\"}");
            }
        } else {
            webServer->send(400, "application/json", "{\"success\":false,\"message\":\"SSID und Passwort erforderlich\"}");
        }
    }
}

void WifiManager::handleScan() {
    String json = "{\"networks\":[";
    
    int n = WiFi.scanNetworks();
    for (int i = 0; i < n; ++i) {
        if (i > 0) json += ",";
        json += "{\"ssid\":\"" + WiFi.SSID(i) + "\",\"rssi\":" + String(WiFi.RSSI(i)) + "}";
    }
    json += "]}";
    
    webServer->send(200, "application/json", json);
}

void WifiManager::handleTestAudio() {
    // Audio-Test über EventManager senden (temporär deaktiviert)
    // if (eventManager) {
    //     eventManager->sendAudioTest();
    //     webServer->send(200, "text/plain", "Test-Ton erfolgreich gespielt");
    // } else {
    //     webServer->send(500, "text/plain", "EventManager nicht verfügbar");
    // }
    webServer->send(200, "text/plain", "Audio-Test deaktiviert");
}

// =============================================================================
// EVENT-INTEGRATION
// =============================================================================

void WifiManager::processConfigUpdate(const String& config) {
    Serial.printf("WifiManager: Verarbeite Konfigurationsupdate: %s\n", config.c_str());
    
    // JSON-Konfiguration parsen und anwenden
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, config);
    
    if (error) {
        Serial.printf("WifiManager: JSON-Parsing-Fehler: %s\n", error.c_str());
        return;
    }
    
    // Neue Konfiguration anwenden
    if (doc.containsKey("ssid")) {
        this->config.ssid = doc["ssid"].as<String>();
    }
    if (doc.containsKey("password")) {
        this->config.password = doc["password"].as<String>();
    }
    if (doc.containsKey("staticIP")) {
        this->config.staticIP = doc["staticIP"].as<String>();
        this->config.useStaticIP = true;
    }
    
    // Konfiguration speichern und neu verbinden
    if (saveConfig()) {
        Serial.println("WifiManager: Neue Konfiguration gespeichert");
        forceReconnect();
    }
}

void WifiManager::handleNotFound() {
    // Captive Portal: Alle unbekannten Anfragen zur Hauptseite umleiten
    webServer->sendHeader("Location", "http://192.168.4.1/", true);
    webServer->send(302, "text/plain", "");
}

bool WifiManager::loadConfigFromNVS() {
    if (!preferences) return false;
    
    config.ssid = preferences->getString(NVS_KEY_WIFI_SSID, "");
    config.password = preferences->getString(NVS_KEY_WIFI_PASS, "");
    config.staticIP = preferences->getString(NVS_KEY_STATIC_IP, "");
    config.staticGateway = preferences->getString(NVS_KEY_STATIC_GW, "");
    config.staticSubnet = preferences->getString(NVS_KEY_STATIC_SM, "");
    config.useStaticIP = preferences->getBool("use_static_ip", false);
    
    Serial.printf("WifiManager: Konfiguration geladen - SSID: %s\n", config.ssid.c_str());
    return config.ssid.length() > 0;
}

bool WifiManager::saveConfigToNVS() {
    if (!preferences) return false;
    
    preferences->putString(NVS_KEY_WIFI_SSID, config.ssid);
    preferences->putString(NVS_KEY_WIFI_PASS, config.password);
    preferences->putString(NVS_KEY_STATIC_IP, config.staticIP);
    preferences->putString(NVS_KEY_STATIC_GW, config.staticGateway);
    preferences->putString(NVS_KEY_STATIC_SM, config.staticSubnet);
    preferences->putBool("use_static_ip", config.useStaticIP);
    
    Serial.printf("WifiManager: Konfiguration gespeichert - SSID: %s\n", config.ssid.c_str());
    return true;
}

void WifiManager::handleDNSServer() {
    if (dnsServer) {
        dnsServer->processNextRequest();
    }
}

// void WifiManager::setEventManager(EventManager* manager) {
//     // eventManager = manager;  // Temporär deaktiviert
//     Serial.println("WifiManager: EventManager gesetzt (deaktiviert)");
// }
