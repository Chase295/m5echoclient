#ifndef WEBSOCKET_CLIENT_H
#define WEBSOCKET_CLIENT_H

#include <Arduino.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include "config.h"

// WebSocket-Verbindungsstatus
enum class WebSocketStatus {
    DISCONNECTED,    // Nicht verbunden
    CONNECTING,      // Verbindungsaufbau läuft
    CONNECTED,       // Verbunden
    RECONNECTING,    // Wiederverbindung läuft
    ERROR            // Fehler aufgetreten
};

// WebSocket-Nachrichten-Typen
enum class MessageType {
    IDENTIFICATION,  // Client-Identifikation
    EVENT,          // Ereignis (z.B. Tastendruck)
    AUDIO,          // Audio-Stream
    COMMAND,        // Server-Befehl
    CONFIG,         // Konfiguration
    OTA,            // OTA-Update
    HEARTBEAT,      // Herzschlag
    UNKNOWN         // Unbekannte Nachricht
};

// WebSocket-Nachricht
struct WebSocketMessage {
    MessageType type;
    String payload;
    bool isBinary;
    size_t binaryLength;
    uint8_t* binaryData;
};

    // AudioChunk wird aus AudioManager.h verwendet

// Callback-Funktionen für Events
typedef void (*WebSocketEventCallback)(WebSocketStatus status);
typedef void (*WebSocketMessageCallback)(const WebSocketMessage& message);
typedef void (*WebSocketAudioCallback)(const uint8_t* data, size_t length);

class WebSocketClient {
private:
    WebSocketStatus currentStatus;
    
    // Server-Konfiguration
    String serverHost;
    int serverPort;
    String clientId;
    String serverUrl;
    
    // Verbindungsverwaltung
    unsigned long lastReconnectAttempt;
    unsigned long lastHeartbeat;
    int reconnectAttempts;
    bool autoReconnect;
    
    // Callback-Funktionen
    WebSocketEventCallback eventCallback;
    WebSocketMessageCallback messageCallback;
    WebSocketAudioCallback audioCallback;
    
    // Nachrichten-Puffer
    String messageBuffer;
    bool messageBufferFull;
    
    // FreeRTOS-Tasks und Synchronisation
    TaskHandle_t webSocketTaskHandle;
    QueueHandle_t messageQueue;
    QueueHandle_t audioQueue;
    SemaphoreHandle_t webSocketMutex;
    
    // Debug und Status
    bool debugEnabled;
    unsigned long lastActivity;
    String lastError;
    
    // WebSocket-spezifische Variablen
    WiFiClient* wifiClient;
    uint8_t* frameBuffer;
    size_t frameBufferSize;
    bool wsConnected;
    
    // Private Methoden
    void processMessage(const String& message);
    void processBinaryMessage(uint8_t* data, size_t length);
    MessageType parseMessageType(const String& message);
    String createIdentificationMessage();
    String createEventMessage(const String& eventType);
    
    // FreeRTOS-Task-Funktionen
    static void webSocketTask(void* parameter);
    
    // Nachrichtenverarbeitung
    void processCommand(const String& message);
    void processConfig(const String& message);
    void processOTA(const String& message);
    
    // WebSocket-spezifische Methoden
    String createWebSocketHandshake();
    String generateWebSocketKey();
    String base64Encode(const String& input);
    bool sendWebSocketFrame(const char* data, size_t length, uint8_t opcode);
    void readWebSocketFrames();

public:
    // Konstruktor & Destruktor
    WebSocketClient();
    ~WebSocketClient();
    
    // Initialisierung
    void begin();
    void update();
    
    // Verbindungssteuerung
    bool connect(const String& host, int port);
    bool connect();
    void disconnect();
    void reconnect();
    WebSocketStatus getStatus() const;
    
    // Server-Konfiguration
    void setServer(const String& host, int port);
    void setClientId(const String& id);
    String getServerHost() const;
    int getServerPort() const;
    String getClientId() const;
    
    // Nachrichten senden
    bool sendMessage(const String& message);
    bool sendEvent(const String& eventType);
    bool sendAudio(const uint8_t* data, size_t length);
    bool sendIdentification();
    bool sendHeartbeat();
    
    // Callback-Registrierung
    void setEventCallback(WebSocketEventCallback callback);
    void setMessageCallback(WebSocketMessageCallback callback);
    void setAudioCallback(WebSocketAudioCallback callback);
    
    // Verbindungsverwaltung
    void setAutoReconnect(bool enabled);
    void setReconnectInterval(unsigned long interval);
    void setHeartbeatInterval(unsigned long interval);
    bool isAutoReconnectEnabled() const;
    
    // Utility-Methoden
    void reset();
    void clearMessageBuffer();
    String getLastError() const;
    unsigned long getLastActivity() const;
    
    // Debug-Methoden
    void printConnectionStatus();
    void printMessageStats();
    void enableDebug(bool enabled);
    
    // Event-Integration
    void sendAudioData(const uint8_t* data, size_t size);
    
    // Öffentliche Methoden für EventManager
    bool isConnected() const;
};

#endif // WEBSOCKET_CLIENT_H
