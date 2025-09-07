# Implementierungsstrategie M5 Echo Client

**Version:** 4.0 (Finaler Bauplan mit Latenz-Kompensation)  
**Datum:** 02.09.2025  
**Status:** Implementierungsbereit

## Übersicht

Dieses Dokument beschreibt die schrittweise Implementierungsstrategie für die M5 Echo Client Firmware. Die Strategie ist in Phasen unterteilt, um eine systematische und testbare Entwicklung zu gewährleisten.

## Phase 1: Projektstruktur und Grundlagen ✅

### Schritt 1: PlatformIO-Projekt erstellen ✅
- [x] PlatformIO installiert
- [x] Projekt mit ESP32-DevKitC-32E Board erstellt
- [x] Grundlegende Konfiguration in `platformio.ini`

### Schritt 2: Projektstruktur aufbauen ✅
- [x] Alle Header-Dateien erstellt
- [x] Klassenarchitektur definiert
- [x] Build-Konfiguration eingerichtet

### Schritt 3: Grundlegende Konfiguration ✅
- [x] `config.h` mit allen Konstanten erstellt
- [x] Hardware-Pins definiert
- [x] WebSocket-API-Spezifikation dokumentiert

## Phase 2: Kernkomponenten-Entwicklung

### Schritt 4: Manager-Klassen implementieren

#### 4.1 LedManager (Priorität: Hoch)
**Zuständigkeit:** ARGB LED-Steuerung mit Zustandsmaschine

**Zu implementierende Methoden:**
```cpp
// Konstruktor & Initialisierung
LedManager::LedManager()
LedManager::~LedManager()
void LedManager::begin()
void LedManager::update()

// Zustandssteuerung
void LedManager::setState(LedState state)
LedState LedManager::getState() const

// Direkte LED-Steuerung
void LedManager::setColor(uint8_t r, uint8_t g, uint8_t b)
void LedManager::setBrightness(uint8_t brightness)
void LedManager::turnOff()
void LedManager::turnOn()

// Spezielle Effekte
void LedManager::startBreathing(uint32_t color)
void LedManager::startPulsing(uint32_t color)
void LedManager::startBlinking(uint32_t color)
void LedManager::stopEffect()
```

**Implementierungsreihenfolge:**
1. Grundlegende LED-Steuerung
2. Zustandsmaschine implementieren
3. Effekte (Breathing, Pulsing, Blinking)
4. Integration mit anderen Managern

#### 4.2 WifiManager (Priorität: Hoch)
**Zuständigkeit:** WLAN-Verbindung und Access-Point-Modus

**Zu implementierende Methoden:**
```cpp
// Initialisierung
void WifiManager::begin()
void WifiManager::update()

// WLAN-Verbindung
bool WifiManager::connect(const String& ssid, const String& password)
bool WifiManager::connect()
void WifiManager::disconnect()
bool WifiManager::isConnected() const
WifiStatus WifiManager::getStatus() const

// Access-Point-Modus
void WifiManager::startAccessPointMode()
void WifiManager::stopAccessPointMode()
bool WifiManager::isAccessPointMode() const

// Konfiguration
void WifiManager::setConfig(const NetworkConfig& config)
NetworkConfig WifiManager::getConfig() const
bool WifiManager::loadConfig()
bool WifiManager::saveConfig()

// Web-Server-Kontrolle
void WifiManager::handleWebServer()
bool WifiManager::isWebServerActive() const
```

**Implementierungsreihenfolge:**
1. Grundlegende WLAN-Verbindung
2. NVS-Speicherung für Konfiguration
3. Access-Point-Modus
4. Web-Server mit Konfigurationsportal
5. Automatische Wiederverbindung

#### 4.3 AudioManager (Priorität: Höchste)
**Zuständigkeit:** I2S Audio-Aufnahme und -Wiedergabe

**Zu implementierende Methoden:**
```cpp
// Initialisierung
bool AudioManager::begin()
void AudioManager::update()

// Mikrofon-Steuerung
bool AudioManager::startRecording()
bool AudioManager::stopRecording()
bool AudioManager::isRecording() const
size_t AudioManager::getAvailableAudio()
size_t AudioManager::readAudio(uint8_t* buffer, size_t maxLength)

// Lautsprecher-Steuerung
bool AudioManager::startPlaying()
bool AudioManager::stopPlaying()
bool AudioManager::isPlaying() const
bool AudioManager::writeAudio(const uint8_t* data, size_t length)
bool AudioManager::writeAudioChunk(const AudioChunk& chunk)

// Puffer-Management
void AudioManager::clearMicBuffer()
void AudioManager::clearSpeakerBuffer()
size_t AudioManager::getMicBufferAvailable() const
size_t AudioManager::getSpeakerBufferAvailable() const
```

**Implementierungsreihenfolge:**
1. I2S-Konfiguration für Mikrofon
2. I2S-Konfiguration für Lautsprecher
3. Ring-Puffer-Implementierung
4. Audio-Chunk-Verarbeitung
5. Stille-Erkennung
6. Latenz-Kompensation

#### 4.4 WebSocketClient (Priorität: Hoch)
**Zuständigkeit:** Echtzeit-Kommunikation mit Server

**Zu implementierende Methoden:**
```cpp
// Initialisierung
void WebSocketClient::begin()
void WebSocketClient::update()

// Verbindungssteuerung
bool WebSocketClient::connect(const String& host, int port)
bool WebSocketClient::connect()
void WebSocketClient::disconnect()
void WebSocketClient::reconnect()
WebSocketStatus WebSocketClient::getStatus() const

// Nachrichten senden
bool WebSocketClient::sendMessage(const String& message)
bool WebSocketClient::sendEvent(const String& eventType)
bool WebSocketClient::sendAudio(const uint8_t* data, size_t length)
bool WebSocketClient::sendIdentification()
bool WebSocketClient::sendHeartbeat()

// Callback-Registrierung
void WebSocketClient::setEventCallback(WebSocketEventCallback callback)
void WebSocketClient::setMessageCallback(WebSocketMessageCallback callback)
void WebSocketClient::setAudioCallback(WebSocketAudioCallback callback)
```

**Implementierungsreihenfolge:**
1. Grundlegende WebSocket-Verbindung
2. Nachrichten-Parsing und -Sending
3. Event-Callbacks
4. Automatische Wiederverbindung
5. Audio-Streaming
6. Heartbeat-Mechanismus

#### 4.5 PowerManager (Priorität: Mittel)
**Zuständigkeit:** Energiemanagement und Deep-Sleep

**Zu implementierende Methoden:**
```cpp
// Initialisierung
void PowerManager::begin()
void PowerManager::update()

// Power-Modus-Steuerung
void PowerManager::setPowerMode(PowerMode mode)
PowerMode PowerManager::getPowerMode() const
PowerState PowerManager::getState() const

// Sleep-Steuerung
void PowerManager::enableSleep(bool enabled)
void PowerManager::setSleepTimeout(unsigned long timeout)
void PowerManager::goToSleep()
void PowerManager::wakeUp()
bool PowerManager::isSleeping() const

// Activity-Tracking
void PowerManager::registerButtonActivity()
void PowerManager::registerAudioActivity()
void PowerManager::registerNetworkActivity()
unsigned long PowerManager::getLastActivityTime() const
bool PowerManager::hasRecentActivity() const
```

**Implementierungsreihenfolge:**
1. Grundlegende Sleep-Funktionalität
2. Wakeup-Quellen konfigurieren
3. Activity-Tracking
4. Sleep-Timer-Logik
5. Integration mit anderen Managern

#### 4.6 OtaManager (Priorität: Niedrig)
**Zuständigkeit:** Over-the-Air Updates

**Zu implementierende Methoden:**
```cpp
// Initialisierung
void OtaManager::begin()
void OtaManager::update()

// Update-Steuerung
bool OtaManager::startUpdate()
bool OtaManager::checkForUpdates()
void OtaManager::cancelUpdate()
OtaStatus OtaManager::getStatus() const

// Auto-Update-Konfiguration
void OtaManager::enableAutoUpdate(bool enabled)
void OtaManager::setCheckInterval(unsigned long interval)
bool OtaManager::isAutoUpdateEnabled() const

// Callback-Registrierung
void OtaManager::setStatusCallback(OtaStatusCallback callback)
void OtaManager::setProgressCallback(OtaProgressCallback callback)
void OtaManager::setErrorCallback(OtaErrorCallback callback)
```

**Implementierungsreihenfolge:**
1. Grundlegende OTA-Funktionalität
2. Update-Server-Kommunikation
3. Sichere Update-Installation
4. Fortschritts-Tracking
5. Auto-Update-Mechanismus

### Schritt 5: Web-Interface
**Zuständigkeit:** Konfigurationsportal im Access-Point-Modus

**Zu implementieren:**
1. HTML/CSS/JavaScript für Konfigurationsportal
2. Formular-Handling
3. Netzwerk-Scan-Funktionalität
4. Validierung und Speicherung
5. Diagnose-Seiten

## Phase 3: Integration und Optimierung

### Schritt 6: Task-Management mit FreeRTOS
**Zuständigkeit:** Parallele Ausführung der Manager-Klassen

**Zu implementieren:**
1. Task-Erstellung und -Verwaltung
2. Inter-Task-Kommunikation (Queues, Semaphores)
3. Task-Prioritäten und Scheduling
4. CPU-Core-Zuweisung
5. Task-Monitoring und Debugging

### Schritt 7: Latenz-Kompensation
**Zuständigkeit:** Intelligente Audio-Pufferung im Deep-Sleep-Modus

**Zu implementieren:**
1. Audio-Pufferung während Verbindungsaufbau
2. Nahtlose Übergang von Puffer zu Live-Stream
3. Vollständige Übertragung nach Taster-Loslassen
4. Optimierung der Puffer-Größen
5. Performance-Monitoring

### Schritt 8: Testing und Debugging
**Zuständigkeit:** Qualitätssicherung und Fehlerbehebung

**Zu implementieren:**
1. Unit-Tests für jede Komponente
2. Integrationstests für Manager-Interaktion
3. Performance-Tests für Audio-Streaming
4. Stress-Tests für Netzwerk-Verbindungen
5. Debug-Ausgaben und Logging

## Implementierungsreihenfolge

### Woche 1: Grundlagen
- [ ] LedManager implementieren
- [ ] WifiManager implementieren
- [ ] Grundlegende Integration testen

### Woche 2: Audio-System
- [ ] AudioManager implementieren
- [ ] I2S-Konfiguration testen
- [ ] Audio-Streaming testen

### Woche 3: Kommunikation
- [ ] WebSocketClient implementieren
- [ ] Server-Kommunikation testen
- [ ] Audio-Streaming über WebSocket testen

### Woche 4: Power-Management
- [ ] PowerManager implementieren
- [ ] Deep-Sleep-Modus testen
- [ ] Latenz-Kompensation implementieren

### Woche 5: Integration
- [ ] FreeRTOS-Tasks integrieren
- [ ] Web-Interface implementieren
- [ ] End-to-End-Tests durchführen

### Woche 6: Optimierung
- [ ] Performance optimieren
- [ ] OTA-Manager implementieren
- [ ] Finale Tests und Dokumentation

## Teststrategie

### Unit-Tests
- Jede Manager-Klasse einzeln testen
- Mock-Objekte für Abhängigkeiten
- Automatisierte Test-Suite

### Integration-Tests
- Manager-Interaktion testen
- End-to-End-Szenarien
- Performance-Metriken

### Hardware-Tests
- M5Stack ATOM Echo Hardware
- Audio-Qualität testen
- LED-Verhalten verifizieren
- Power-Management validieren

## Risiken und Mitigation

### Risiko: Audio-Latenz zu hoch
**Mitigation:** Frühe Performance-Tests, Puffer-Optimierung

### Risiko: WebSocket-Verbindung instabil
**Mitigation:** Robuste Wiederverbindungslogik, Heartbeat-Mechanismus

### Risiko: Deep-Sleep verursacht Datenverlust
**Mitigation:** Sorgfältige Puffer-Verwaltung, Vollständige Übertragung

### Risiko: Memory-Leaks in FreeRTOS-Tasks
**Mitigation:** Memory-Monitoring, Task-Cleanup

## Erfolgskriterien

### Funktionale Kriterien
- [ ] Audio-Streaming funktioniert ohne Unterbrechungen
- [ ] LED zeigt korrekte Zustände an
- [ ] WLAN-Verbindung ist stabil
- [ ] Deep-Sleep-Modus funktioniert
- [ ] Web-Interface ist benutzerfreundlich

### Performance-Kriterien
- [ ] Audio-Latenz < 100ms
- [ ] WebSocket-Verbindung < 5s Aufbauzeit
- [ ] Deep-Sleep-Wakeup < 1s
- [ ] Memory-Usage < 80% verfügbar

### Qualitätskriterien
- [ ] Code-Coverage > 80%
- [ ] Keine Memory-Leaks
- [ ] Robuste Fehlerbehandlung
- [ ] Vollständige Dokumentation

## Nächste Schritte

1. **Sofort:** LedManager implementieren
2. **Parallel:** WifiManager implementieren
3. **Danach:** AudioManager implementieren
4. **Weiter:** WebSocketClient implementieren
5. **Abschließend:** Integration und Optimierung

Diese Implementierungsstrategie gewährleistet eine systematische, testbare und qualitativ hochwertige Entwicklung der M5 Echo Client Firmware.
