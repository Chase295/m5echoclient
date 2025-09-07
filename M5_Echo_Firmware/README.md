# M5 Echo Client Firmware

**Version:** 4.0 (Finaler Bauplan mit Latenz-Kompensation)  
**Datum:** 02.09.2025  
**Status:** Implementierungsbereit

## Übersicht

Die Firmware für den M5 Echo Client implementiert einen hocheffizienten, streaming-fähigen und intelligenten "Thin Client" für das dezentrale KI-Assistenz-System. Das System ist als dezentrales Nervensystem konzipiert, wobei der M5 Echo als Sinnesorgan (Ohr und Mund) fungiert.

## Architektur

### Kernprinzipien

- **Streaming-First**: Optimiert für kontinuierliche Audio-Datenströme in kleinen Paketen
- **Zustandslose Logik**: Führt lediglich Befehle vom Server aus
- **Visuelles Feedback**: Klare Zustandsanzeige über ARGB-LED
- **Persistente Konfiguration**: Einstellungen werden im NVS gespeichert
- **Energieeffizienz**: Unterstützt Dauerbetrieb und Deep-Sleep-Modus

### Hardware-Spezifikation

- **Mikrocontroller**: ESP32-PICO-D4
- **Mikrofon (I2S)**: SPM1423
- **Lautsprecher (I2S)**: NS4168
- **LED (ARGB)**: 1x, GPIO 27
- **Taste**: GPIO 39

## Projektstruktur

```
M5_Echo_Firmware/
├── platformio.ini          # PlatformIO-Konfiguration
├── src/
│   ├── main.cpp           # Hauptanwendungslogik
│   ├── config.h           # Zentrale Konfiguration
│   ├── LedManager.h       # ARGB LED-Steuerung
│   ├── WifiManager.h      # WLAN-Verbindung & Access-Point
│   ├── AudioManager.h     # I2S Audio-Aufnahme/-Wiedergabe
│   ├── WebSocketClient.h  # Echtzeit-Kommunikation
│   ├── PowerManager.h     # Energiemanagement
│   └── OtaManager.h       # Over-the-Air Updates
└── README.md              # Diese Datei
```

## Manager-Klassen

### LedManager
Verwaltet die ARGB-LED mit einer präzisen Zustandsmaschine:

| Zustand | LED-Farbe | LED-Effekt |
|---------|-----------|------------|
| Booten | Weiß | Kurz aufleuchten |
| Access-Point-Modus | Blau | Pulsieren |
| WLAN-Verbindungsaufbau | Cyan | Blinken |
| Server-Verbindungsaufbau | Grün | Blinken |
| Verbunden & Bereit | (Aus oder dezentes Grün) | Statisch |
| Fehler | Rot | Statisch |
| Taster gedrückt | Weiß | Kurz aufleuchten |
| Mikrofon aktiv | Cyan | Atmen |
| Audio-Wiedergabe | Grün | Pulsieren |
| OTA Update | Violett | Pulsieren |

### WifiManager
Verwaltet WLAN-Verbindung und Access-Point-Modus:
- Automatische WLAN-Verbindung
- Access-Point-Modus für Konfiguration
- Web-basiertes Konfigurationsportal
- NVS-Speicherung für Einstellungen

### AudioManager
Verwaltet I2S Audio-Aufnahme und -Wiedergabe:
- Kontinuierliche Audio-Streaming
- Ring-Puffer für Latenz-Kompensation
- Stille-Erkennung
- Audio-Chunk-Verarbeitung

### WebSocketClient
Verwaltet die Echtzeit-Kommunikation mit dem Server:
- Automatische Wiederverbindung
- Nachrichten-Parsing
- Audio-Streaming
- Event-Callbacks

### PowerManager
Verwaltet das Energiemanagement:
- Deep-Sleep-Modus
- Taster-Wakeup
- Activity-Tracking
- Sleep-Timer

### OtaManager
Verwaltet Over-the-Air Updates:
- Automatische Update-Prüfung
- Sichere Update-Installation
- Fortschritts-Tracking
- Rollback-Funktionalität

## Betriebsmodi

### Dauerbetrieb (microphone_mode: "always_on")
- Gerät bleibt permanent aktiv
- Kontinuierliche Audio-Aufnahme
- Sofortige Reaktion auf Server-Befehle

### Ereignisbasierter Modus (microphone_mode: "on_button_press")
- Intelligente Latenz-Kompensation
- Deep-Sleep für Energieeffizienz
- Sofortige Audio-Aufnahme bei Tastendruck
- Pufferung während Verbindungsaufbau

## WebSocket API

### Client → Server
- **identification**: Client-Identifikation mit Fähigkeiten
- **event**: Ereignisse wie Tastendrücke
- **audio**: Rohe Audio-Chunks vom Mikrofon

### Server → Client
- **command**: LED-Steuerung und Gerätebefehle
- **config**: Konfigurationsänderungen
- **ota**: OTA-Update-Befehle
- **audio**: Rohe Audio-Chunks zur Wiedergabe

## Konfiguration

Das Web-Interface im Access-Point-Modus bietet:

- **WLAN-Einstellungen**: SSID, Passwort, Netzwerk-Scan
- **Netzwerk-Einstellungen**: Optionale feste IP-Adresse
- **Server-Einstellungen**: IP/Hostname, Port
- **Geräte-ID**: Eindeutige Client-Identifikation
- **Mikrofon-Modus**: "Immer aktiv" oder "Nur bei Tastendruck"
- **Diagnose**: Systemstatus und Test-Funktionen

## FreeRTOS Task-Management

| Task | Priorität | Stack-Größe | CPU-Core | Funktion |
|------|-----------|-------------|----------|----------|
| AudioTask | 3 (Höchste) | 8192 | 1 | Audio-Verarbeitung |
| WebSocketTask | 2 (Hoch) | 4096 | 0 | WebSocket-Kommunikation |
| LedTask | 1 (Normal) | 2048 | 0 | LED-Steuerung |
| WifiTask | 1 (Normal) | 4096 | 0 | WLAN-Verwaltung |
| PowerTask | 0 (Niedrig) | 2048 | 0 | Power-Management |

## Build und Upload

### Voraussetzungen
- PlatformIO IDE oder PlatformIO CLI
- ESP32-Entwicklungsumgebung
- USB-Kabel für Upload

### Build
```bash
cd M5_Echo_Firmware
pio run
```

### Upload
```bash
pio run --target upload
```

### Monitor
```bash
pio device monitor
```

## Entwicklung

### Debugging
- Serial-Monitor für Debug-Ausgaben
- LED-Zustände für visuelle Diagnose
- Web-Interface für Konfiguration

### Erweiterungen
- Wake-Word-Erkennung
- Client-Gruppierung
- Multi-Modalität (Kamera)
- Erweiterte Hausautomation

## Lizenz

Dieses Projekt ist Teil des dezentralen KI-Assistenz-Systems und unterliegt den entsprechenden Lizenzbedingungen.

## Support

Bei Fragen oder Problemen siehe die vollständige Spezifikation in `M5_ECHO_SPEZIFIKATION.md`.
