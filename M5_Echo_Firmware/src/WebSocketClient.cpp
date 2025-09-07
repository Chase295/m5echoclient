#include "WebSocketClient.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <WiFiClient.h>
#include "AudioManager.h"  // Für AudioChunk-Definition

// =============================================================================
// KONSTRUKTOR & DESTRUKTOR
// =============================================================================

WebSocketClient::WebSocketClient() {
    wifiClient = nullptr;
    currentStatus = WebSocketStatus::DISCONNECTED;
    
    // Server-Konfiguration
    serverHost = DEFAULT_SERVER_HOST;
    serverPort = DEFAULT_SERVER_PORT;
    clientId = DEFAULT_CLIENT_ID;
    serverUrl = "";
    
    // Verbindungsverwaltung
    lastReconnectAttempt = 0;
    lastHeartbeat = 0;
    reconnectAttempts = 0;
    autoReconnect = true;
    
    // Callback-Funktionen
    eventCallback = nullptr;
    messageCallback = nullptr;
    audioCallback = nullptr;
    
    // Nachrichten-Puffer
    messageBuffer = "";
    messageBufferFull = false;
    
    // FreeRTOS-Tasks und Synchronisation
    webSocketTaskHandle = nullptr;
    messageQueue = nullptr;
    audioQueue = nullptr;
    webSocketMutex = nullptr;
    
    // Debug
    debugEnabled = false;
    lastActivity = 0;
    lastError = "";
    
    // WebSocket-spezifische Variablen
    frameBuffer = nullptr;
    frameBufferSize = 0;
    wsConnected = false;
}

WebSocketClient::~WebSocketClient() {
    // Tasks stoppen
    if (webSocketTaskHandle) {
        vTaskDelete(webSocketTaskHandle);
        webSocketTaskHandle = nullptr;
    }
    
    // Queues löschen
    if (messageQueue) {
        vQueueDelete(messageQueue);
        messageQueue = nullptr;
    }
    if (audioQueue) {
        vQueueDelete(audioQueue);
        audioQueue = nullptr;
    }
    
    // Mutex löschen
    if (webSocketMutex) {
        vSemaphoreDelete(webSocketMutex);
        webSocketMutex = nullptr;
    }
    
    // WiFi-Client löschen
    if (wifiClient) {
        delete wifiClient;
        wifiClient = nullptr;
    }
    
    // Frame-Buffer freigeben
    if (frameBuffer) {
        free(frameBuffer);
        frameBuffer = nullptr;
    }
}

// =============================================================================
// INITIALISIERUNG
// =============================================================================

void WebSocketClient::begin() {
    Serial.println("WebSocketClient: Initialisiere...");
    
    // Mutex erstellen
    webSocketMutex = xSemaphoreCreateMutex();
    if (!webSocketMutex) {
        Serial.println("WebSocketClient: Fehler beim Erstellen des Mutex");
        return;
    }
    
    // WiFi-Client erstellen
    wifiClient = new WiFiClient();
    if (!wifiClient) {
        Serial.println("WebSocketClient: Fehler beim Erstellen des WiFi-Clients");
        return;
    }
    
    // Frame-Buffer allozieren
    frameBufferSize = 4096;
    frameBuffer = (uint8_t*)malloc(frameBufferSize);
    if (!frameBuffer) {
        Serial.println("WebSocketClient: Fehler beim Allozieren des Frame-Buffers");
        return;
    }
    
    // Queues erstellen
    messageQueue = xQueueCreate(20, sizeof(WebSocketMessage));
    audioQueue = xQueueCreate(10, sizeof(AudioChunk));
    
    if (!messageQueue || !audioQueue) {
        Serial.println("WebSocketClient: Fehler beim Erstellen der Queues");
        return;
    }
    
    // WebSocket-Task starten
    BaseType_t result = xTaskCreatePinnedToCore(
        webSocketTask,
        "WebSocketTask",
        8192,
        this,
        WEBSOCKET_TASK_PRIORITY,
        &webSocketTaskHandle,
        0  // Core 0 für Netzwerk-Verarbeitung
    );
    
    if (result != pdPASS) {
        Serial.println("WebSocketClient: Fehler beim Erstellen der WebSocket-Task");
        return;
    }
    
    currentStatus = WebSocketStatus::DISCONNECTED;
    Serial.println("WebSocketClient: Initialisierung abgeschlossen");
}

void WebSocketClient::update() {
    // Automatische Wiederverbindung
    if (autoReconnect && currentStatus == WebSocketStatus::DISCONNECTED) {
        unsigned long currentTime = millis();
        if (currentTime - lastReconnectAttempt > WS_RECONNECT_INTERVAL) {
            if (reconnectAttempts < 5) {
                Serial.println("WebSocketClient: Automatische Wiederverbindung...");
                connect();
                reconnectAttempts++;
            } else {
                Serial.println("WebSocketClient: Maximale Reconnect-Versuche erreicht");
                currentStatus = WebSocketStatus::ERROR;
            }
            lastReconnectAttempt = currentTime;
        }
    }
    
    // Heartbeat senden
    if (currentStatus == WebSocketStatus::CONNECTED) {
        unsigned long currentTime = millis();
        if (currentTime - lastHeartbeat > WS_HEARTBEAT_INTERVAL) {
            sendHeartbeat();
            lastHeartbeat = currentTime;
        }
    }
}

// =============================================================================
// VERBINDUNGSSTEUERUNG
// =============================================================================

bool WebSocketClient::connect(const String& host, int port) {
    if (xSemaphoreTake(webSocketMutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return false;
    }
    
    serverHost = host;
    serverPort = port;
    serverUrl = "ws://" + host + ":" + String(port);
    
    Serial.printf("WebSocketClient: Verbinde mit %s...\n", serverUrl.c_str());
    
    currentStatus = WebSocketStatus::CONNECTING;
    
    // TCP-Verbindung herstellen
    if (!wifiClient->connect(host.c_str(), port)) {
        Serial.println("WebSocketClient: TCP-Verbindung fehlgeschlagen");
        currentStatus = WebSocketStatus::ERROR;
        xSemaphoreGive(webSocketMutex);
        return false;
    }
    
    // WebSocket-Handshake senden
    String handshake = createWebSocketHandshake();
    wifiClient->print(handshake);
    
    // Handshake-Antwort lesen
    String response = "";
    unsigned long timeout = millis() + 5000; // 5 Sekunden Timeout
    
    while (millis() < timeout && !response.endsWith("\r\n\r\n")) {
        if (wifiClient->available()) {
            response += (char)wifiClient->read();
        }
        delay(1);
    }
    
    if (response.indexOf("101 Switching Protocols") > 0) {
        Serial.println("WebSocketClient: WebSocket-Handshake erfolgreich");
        currentStatus = WebSocketStatus::CONNECTED;
        wsConnected = true;
        reconnectAttempts = 0;
        lastActivity = millis();
        
        // Identifikation senden
        sendIdentification();
        
        if (eventCallback) {
            eventCallback(WebSocketStatus::CONNECTED);
        }
    } else {
        Serial.println("WebSocketClient: WebSocket-Handshake fehlgeschlagen");
        currentStatus = WebSocketStatus::ERROR;
        wifiClient->stop();
    }
    
    xSemaphoreGive(webSocketMutex);
    return currentStatus == WebSocketStatus::CONNECTED;
}

bool WebSocketClient::connect() {
    return connect(serverHost, serverPort);
}

void WebSocketClient::disconnect() {
    if (xSemaphoreTake(webSocketMutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return;
    }
    
    if (wifiClient) {
        wifiClient->stop();
    }
    
    currentStatus = WebSocketStatus::DISCONNECTED;
    wsConnected = false;
    reconnectAttempts = 0;
    
    xSemaphoreGive(webSocketMutex);
    Serial.println("WebSocketClient: Verbindung getrennt");
}

void WebSocketClient::reconnect() {
    disconnect();
    delay(1000);
    connect();
}

WebSocketStatus WebSocketClient::getStatus() const {
    return currentStatus;
}

// =============================================================================
// SERVER-KONFIGURATION
// =============================================================================

void WebSocketClient::setServer(const String& host, int port) {
    serverHost = host;
    serverPort = port;
    serverUrl = "ws://" + host + ":" + String(port);
}

void WebSocketClient::setClientId(const String& id) {
    clientId = id;
}

String WebSocketClient::getServerHost() const {
    return serverHost;
}

int WebSocketClient::getServerPort() const {
    return serverPort;
}

String WebSocketClient::getClientId() const {
    return clientId;
}

// =============================================================================
// NACHRICHTEN SENDEN
// =============================================================================

bool WebSocketClient::sendMessage(const String& message) {
    if (!(currentStatus == WebSocketStatus::CONNECTED && wifiClient && wifiClient->connected())) {
        return false;
    }
    
    if (xSemaphoreTake(webSocketMutex, pdMS_TO_TICKS(10)) != pdTRUE) {
        return false;
    }
    
    bool result = sendWebSocketFrame(message.c_str(), message.length(), 0x01); // Text-Frame
    if (result) {
        lastActivity = millis();
    }
    
    xSemaphoreGive(webSocketMutex);
    return result;
}

bool WebSocketClient::sendEvent(const String& eventType) {
    String message = createEventMessage(eventType);
    return sendMessage(message);
}

bool WebSocketClient::sendAudio(const uint8_t* data, size_t length) {
    if (!(currentStatus == WebSocketStatus::CONNECTED && wifiClient && wifiClient->connected()) || !data || length == 0) {
        return false;
    }
    
    if (xSemaphoreTake(webSocketMutex, pdMS_TO_TICKS(10)) != pdTRUE) {
        return false;
    }
    
    bool result = sendWebSocketFrame((const char*)data, length, 0x02); // Binary-Frame
    if (result) {
        lastActivity = millis();
    }
    
    xSemaphoreGive(webSocketMutex);
    return result;
}

bool WebSocketClient::sendIdentification() {
    String message = createIdentificationMessage();
    return sendMessage(message);
}

bool WebSocketClient::sendHeartbeat() {
    String heartbeat = "{\"type\":\"heartbeat\",\"clientId\":\"" + clientId + "\",\"timestamp\":" + String(millis()) + "}";
    return sendMessage(heartbeat);
}

// =============================================================================
// CALLBACK-REGISTRIERUNG
// =============================================================================

void WebSocketClient::setEventCallback(WebSocketEventCallback callback) {
    eventCallback = callback;
}

void WebSocketClient::setMessageCallback(WebSocketMessageCallback callback) {
    messageCallback = callback;
}

void WebSocketClient::setAudioCallback(WebSocketAudioCallback callback) {
    audioCallback = callback;
}

// =============================================================================
// VERBINDUNGSVERWALTUNG
// =============================================================================

void WebSocketClient::setAutoReconnect(bool enabled) {
    autoReconnect = enabled;
}

void WebSocketClient::setReconnectInterval(unsigned long interval) {
    // Wird in update() verwendet
}

void WebSocketClient::setHeartbeatInterval(unsigned long interval) {
    // Wird in update() verwendet
}

bool WebSocketClient::isAutoReconnectEnabled() const {
    return autoReconnect;
}

// =============================================================================
// UTILITY-METHODEN
// =============================================================================

void WebSocketClient::reset() {
    disconnect();
    reconnectAttempts = 0;
    clearMessageBuffer();
    lastActivity = 0;
    lastError = "";
}

void WebSocketClient::clearMessageBuffer() {
    messageBuffer = "";
    messageBufferFull = false;
}

String WebSocketClient::getLastError() const {
    return lastError;
}

unsigned long WebSocketClient::getLastActivity() const {
    return lastActivity;
}

// =============================================================================
// DEBUG-METHODEN
// =============================================================================

void WebSocketClient::printConnectionStatus() {
    Serial.printf("WebSocketClient: Status: %d, Server: %s:%d, ClientID: %s\n",
                  (int)currentStatus,
                  serverHost.c_str(),
                  serverPort,
                  clientId.c_str());
}

void WebSocketClient::printMessageStats() {
    Serial.printf("WebSocketClient: Reconnect-Versuche: %d, Letzte Aktivität: %lu ms\n",
                  reconnectAttempts,
                  lastActivity);
}

void WebSocketClient::enableDebug(bool enabled) {
    debugEnabled = enabled;
}

// =============================================================================
// PRIVATE METHODEN
// =============================================================================

String WebSocketClient::createWebSocketHandshake() {
    String key = generateWebSocketKey();
    String handshake = "GET / HTTP/1.1\r\n";
    handshake += "Host: " + serverHost + ":" + String(serverPort) + "\r\n";
    handshake += "Upgrade: websocket\r\n";
    handshake += "Connection: Upgrade\r\n";
    handshake += "Sec-WebSocket-Key: " + key + "\r\n";
    handshake += "Sec-WebSocket-Version: 13\r\n";
    handshake += "\r\n";
    return handshake;
}

String WebSocketClient::generateWebSocketKey() {
    // Einfache WebSocket-Key-Generierung
    String key = "";
    for (int i = 0; i < 16; i++) {
        key += (char)(random(65, 91)); // A-Z
    }
    return base64Encode(key);
}

String WebSocketClient::base64Encode(const String& input) {
    // Vereinfachte Base64-Kodierung
    const char* chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    String result = "";
    int i = 0;
    
    while (i < input.length()) {
        uint32_t octet_a = i < input.length() ? input[i++] : 0;
        uint32_t octet_b = i < input.length() ? input[i++] : 0;
        uint32_t octet_c = i < input.length() ? input[i++] : 0;
        
        uint32_t triple = (octet_a << 16) + (octet_b << 8) + octet_c;
        
        result += chars[(triple >> 18) & 0x3F];
        result += chars[(triple >> 12) & 0x3F];
        result += chars[(triple >> 6) & 0x3F];
        result += chars[triple & 0x3F];
    }
    
    return result;
}

bool WebSocketClient::sendWebSocketFrame(const char* data, size_t length, uint8_t opcode) {
    if (!wifiClient || !wifiClient->connected()) {
        return false;
    }
    
    // WebSocket-Frame erstellen
    uint8_t frame[10];
    int frameSize = 0;
    
    // FIN + RSV + Opcode
    frame[frameSize++] = 0x80 | opcode;
    
    // Payload-Length
    if (length < 126) {
        frame[frameSize++] = length;
    } else if (length < 65536) {
        frame[frameSize++] = 126;
        frame[frameSize++] = (length >> 8) & 0xFF;
        frame[frameSize++] = length & 0xFF;
    } else {
        frame[frameSize++] = 127;
        for (int i = 7; i >= 0; i--) {
            frame[frameSize++] = (length >> (i * 8)) & 0xFF;
        }
    }
    
    // Frame senden
    wifiClient->write(frame, frameSize);
    
    // Daten senden
    wifiClient->write((uint8_t*)data, length);
    
    return true;
}



void WebSocketClient::processMessage(const String& message) {
    MessageType type = parseMessageType(message);
    
    WebSocketMessage wsMessage;
    wsMessage.type = type;
    wsMessage.payload = message;
    wsMessage.isBinary = false;
    wsMessage.binaryLength = 0;
    wsMessage.binaryData = nullptr;
    
    // Nachricht in Queue für Verarbeitung
    if (messageQueue && xQueueSend(messageQueue, &wsMessage, pdMS_TO_TICKS(10)) == pdTRUE) {
        // Nachricht erfolgreich in Queue gesendet
    }
    
    // Callback aufrufen
    if (messageCallback) {
        messageCallback(wsMessage);
    }
    
    // Spezifische Nachrichtenverarbeitung
    switch (type) {
        case MessageType::COMMAND:
            processCommand(message);
            break;
        case MessageType::CONFIG:
            processConfig(message);
            break;
        case MessageType::OTA:
            processOTA(message);
            break;
        default:
            break;
    }
}

void WebSocketClient::processBinaryMessage(uint8_t* data, size_t length) {
    // Audio-Daten an AudioManager weiterleiten
    if (audioCallback) {
        audioCallback(data, length);
    }
    
    // Audio-Chunk in Queue für Verarbeitung
    AudioChunk audioChunk;
    audioChunk.data = data;
    audioChunk.length = length;
    audioChunk.timestamp = millis();
    audioChunk.isSilence = false;
    
    if (audioQueue && xQueueSend(audioQueue, &audioChunk, pdMS_TO_TICKS(10)) == pdTRUE) {
        // Audio-Chunk erfolgreich in Queue gesendet
    }
}

MessageType WebSocketClient::parseMessageType(const String& message) {
    // Einfache JSON-Parsing für Nachrichten-Typ
    if (message.indexOf("\"type\":\"identification\"") > 0) {
        return MessageType::IDENTIFICATION;
    } else if (message.indexOf("\"type\":\"event\"") > 0) {
        return MessageType::EVENT;
    } else if (message.indexOf("\"type\":\"audio\"") > 0) {
        return MessageType::AUDIO;
    } else if (message.indexOf("\"type\":\"command\"") > 0) {
        return MessageType::COMMAND;
    } else if (message.indexOf("\"type\":\"config\"") > 0) {
        return MessageType::CONFIG;
    } else if (message.indexOf("\"type\":\"ota\"") > 0) {
        return MessageType::OTA;
    } else if (message.indexOf("\"type\":\"heartbeat\"") > 0) {
        return MessageType::HEARTBEAT;
    } else {
        return MessageType::UNKNOWN;
    }
}

String WebSocketClient::createIdentificationMessage() {
    String message = "{\"type\":\"identification\",\"clientId\":\"" + clientId + "\",";
    message += "\"capabilities\":{\"audio\":true,\"led\":true,\"button\":true},";
    message += "\"version\":\"1.0.0\",\"timestamp\":" + String(millis()) + "}";
    return message;
}

String WebSocketClient::createEventMessage(const String& eventType) {
    String message = "{\"type\":\"event\",\"event\":\"" + eventType + "\",";
    message += "\"clientId\":\"" + clientId + "\",\"timestamp\":" + String(millis()) + "}";
    return message;
}

bool WebSocketClient::isConnected() const {
    return currentStatus == WebSocketStatus::CONNECTED && wifiClient && wifiClient->connected();
}

// =============================================================================
// NACHRICHTENVERARBEITUNG
// =============================================================================

void WebSocketClient::processCommand(const String& message) {
    // JSON-Parsing für Befehle
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, message);
    
    if (error) {
        Serial.println("WebSocketClient: JSON-Parsing-Fehler");
        return;
    }
    
    String command = doc["command"] | "";
    String target = doc["target"] | "";
    
    if (command == "led") {
        // LED-Befehl an LedManager weiterleiten
        String color = doc["color"] | "";
        String effect = doc["effect"] | "";
        // Hier würde die Integration mit LedManager erfolgen
        Serial.printf("WebSocketClient: LED-Befehl - Farbe: %s, Effekt: %s\n", color.c_str(), effect.c_str());
    }
}

void WebSocketClient::processConfig(const String& message) {
    // Konfigurationsänderungen verarbeiten
    Serial.println("WebSocketClient: Konfiguration empfangen");
    // Hier würde die Integration mit anderen Managern erfolgen
}

void WebSocketClient::processOTA(const String& message) {
    // OTA-Update-Befehle verarbeiten
    Serial.println("WebSocketClient: OTA-Update-Befehl empfangen");
    // Hier würde die Integration mit OtaManager erfolgen
}

// =============================================================================
// FREERTOS-TASK
// =============================================================================

void WebSocketClient::webSocketTask(void* parameter) {
    WebSocketClient* client = static_cast<WebSocketClient*>(parameter);
    
    Serial.println("WebSocketClient: WebSocket-Task gestartet");
    
    while (true) {
        // WebSocket-Frames lesen
        if (client->wifiClient && client->wifiClient->connected()) {
            client->readWebSocketFrames();
        }
        
        // Nachrichten aus Queue verarbeiten
        WebSocketMessage message;
        if (client->messageQueue && xQueueReceive(client->messageQueue, &message, pdMS_TO_TICKS(10)) == pdTRUE) {
            // Nachricht verarbeiten
            if (client->debugEnabled) {
                Serial.printf("WebSocketClient: Verarbeite Nachricht vom Typ %d\n", (int)message.type);
            }
        }
        
        // Audio-Chunks aus Queue verarbeiten
        AudioChunk audioChunk;
        if (client->audioQueue && xQueueReceive(client->audioQueue, &audioChunk, pdMS_TO_TICKS(10)) == pdTRUE) {
            // Audio-Chunk an AudioManager weiterleiten
            if (client->debugEnabled) {
                Serial.printf("WebSocketClient: Verarbeite Audio-Chunk: %d Bytes\n", audioChunk.length);
            }
        }
        
        // Kurze Pause für andere Tasks
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// =============================================================================
// EVENT-INTEGRATION
// =============================================================================

void WebSocketClient::sendAudioData(const uint8_t* data, size_t size) {
    if (!isConnected() || !data || size == 0) {
        return;
    }
    
    // Audio-Daten als binäre WebSocket-Nachricht senden
    sendAudio(data, size);
    
    if (debugEnabled) {
        Serial.printf("WebSocketClient: Audio-Daten gesendet (%d Bytes)\n", size);
    }
}

void WebSocketClient::readWebSocketFrames() {
    if (!wifiClient || !wifiClient->connected()) {
        return;
    }
    
    // Vereinfachte WebSocket-Frame-Verarbeitung
    if (wifiClient->available()) {
        uint8_t byte = wifiClient->read();
        
        // Einfache Text-Nachrichten verarbeiten
        if (byte >= 32 && byte <= 126) { // Druckbare ASCII-Zeichen
            messageBuffer += (char)byte;
            
            if (messageBuffer.endsWith("}")) {
                // Vollständige JSON-Nachricht empfangen
                processMessage(messageBuffer);
                messageBuffer = "";
            }
        }
    }
}

// =============================================================================
// FEHLENDE MEMBER-VARIABLEN IN HEADER HINZUFÜGEN
// =============================================================================

// Diese Variablen müssen in der Header-Datei hinzugefügt werden:
// WiFiClient* wifiClient;
// uint8_t* frameBuffer;
// size_t frameBufferSize;
// bool isConnected;
