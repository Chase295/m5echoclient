1. Vision & Architektur (Firmware)
1.1. Zielsetzung
Die Firmware implementiert einen hocheffizienten, streaming-fähigen und intelligenten "Thin Client". Seine Aufgabe ist die Verwaltung der lokalen Hardware, die Aufrechterhaltung einer stabilen Echtzeit-Audio-Streaming-Verbindung und die Ausführung von Server-Befehlen.
1.2. Architektonische Kernprinzipien
- Streaming-First: Die Firmware ist darauf optimiert, kontinuierliche Audio-Datenströme in kleinen Paketen (Chunks) zu senden und zu empfangen, um Latenz zu minimieren.
- Zustandslose Logik: Die Firmware führt lediglich Befehle vom Server aus. Die gesamte Anwendungslogik residiert auf dem Server.
- Visuelles Feedback: Der Gerätezustand wird dem Benutzer jederzeit klar und intuitiv über die ARGB-LED signalisiert.
- Persistente Konfiguration: Wichtige Einstellungen wie WLAN-Zugangsdaten werden persistent im NVS (Non-Volatile Storage) gespeichert.
- Energieeffizienz: Das Gerät unterstützt sowohl einen Dauerbetriebsmodus als auch einen extrem stromsparenden Deep Sleep Modus mit intelligenter Latenz-Kompensation.
1.3. Hardware-Spezifikation
- Mikrocontroller: ESP32-PICO-D4
- Mikrofon (I2S): SPM1423
- Lautsprecher (I2S): NS4168
- LED (ARGB): 1x, GPIO 27
- Taste: GPIO 39
1. Betriebsablauf & Energiemanagement
Der Betriebsmodus ist direkt an den konfigurierten microphone_mode gekoppelt.
Dauerbetrieb (microphone_mode: "always_on")
- Start & Standby: Das Gerät verbindet sich mit WLAN und dem WebSocket-Server und bleibt permanent aktiv und empfangsbereit.
- Interaktion: Audio wird gestreamt, sobald der Server dies anfordert.
Ereignisbasierter Modus (microphone_mode: "on_button_press")
Dieser Modus nutzt eine intelligente Pufferung, um die Verbindungslatenz für den Benutzer zu eliminieren.
- Schlaf: Das Gerät befindet sich im Deep Sleep.
- Aufwachen (Tastendruck):
    - Das Gerät wacht durch den Tasten-Interrupt auf.
    - Sofortiges Optisches Feedback: Der LedManager schaltet die LED unmittelbar auf "Taster gedrückt" (z.B. Weiß).
    - Sofortige Audio-Aufnahme: Der AudioManager beginnt sofort mit der Aufnahme des Mikrofons und schreibt die Audio-Chunks in einen Ringpuffer im RAM.
    - Paralleler Verbindungsaufbau: Gleichzeitig starten WifiManager und WebSocketClient den Verbindungsaufbau im Hintergrund.
- Verbindung hergestellt:
    - Sobald die WebSocket-Verbindung steht, sendet der Client seine identification-Nachricht.
    - Puffer Übertragen: Der WebSocketClient sendet zuerst den gesamten Inhalt des Audio-Puffers an den Server.
    - Live-Streaming: Danach schaltet er nahtlos auf den Live-Stream der neuen Audio-Chunks vom Mikrofon um.
- Interaktions-Ende (Taster losgelassen):
    - Der AudioManager stoppt die Aufnahme vom Mikrofon.
    - Vollständige Übertragung: Der WebSocketClient arbeitet weiter, bis alle gepufferten und live aufgenommenen Audio-Chunks vollständig an den Server übertragen wurden. Erst dann gilt die Sendung als abgeschlossen.
- Zurück zum Schlaf: Sobald die Übertragung komplett ist, versetzt der PowerManager das Gerät wieder in den Deep Sleep.
1. Projektstruktur (PlatformIO)
M5_Echo_Firmware/
├── platformio.ini
├── src/
│ ├── main.cpp
│ ├── config.h
│ ├── WifiManager.h/.cpp
│ ├── WebSocketClient.h/.cpp
│ ├── AudioManager.h/.cpp
│ ├── LedManager.h/.cpp
│ ├── PowerManager.h/.cpp
│ └── OtaManager.h/.cpp
└── data/
└── ...
2. Definitive WebSocket API-Spezifikation
4.1. Nachrichten vom M5 Echo an den Server
- Identifikation: (JSON) Wie spezifiziert.
- Tastendruck-Ereignis: (JSON) {"type": "event", "source": "button", "action": "pressed"}
- Audio-Stream: (Binary) Rohe Audio-Chunks.
4.2. Nachrichten vom Server an den M5 Echo
- LED-Steuerung: (JSON) Wie spezifiziert.
- Gerätekonfiguration: (JSON) Wie spezifiziert.
- OTA Update: (JSON) Wie spezifiziert.
- Audio-Stream: (Binary) Rohe Audio-Chunks.
1. Statusanzeige (ARGB LED)
Der LedManager implementiert exakt die folgende Zustandsmaschine:
| Zustand | LED-Farbe | LED-Effekt |
|---|---|---|
| Booten / Startvorgang | Weiß | Kurz aufleuchten |
| Access-Point-Modus | Blau | Pulsieren |
| WLAN-Verbindungsaufbau | Cyan | Blinken |
| Server-Verbindungsaufbau | Grün | Blinken |
| Verbunden & Bereit | (Aus oder dezentes Grün) | Statisch |
| Fehler (Kein WLAN/Server) | Rot | Statisch |
| Taster gedrückt | Weiß | Kurz aufleuchten |
| Mikrofon aktiv / Hört zu | Cyan | Atmen |
| Audio-Wiedergabe | Grün | Pulsieren |
| OTA Update läuft | Violett | Pulsieren |
2. Web-Interface (Konfigurationsportal)
Wird im Access-Point-Modus bereitgestellt und bietet folgende Einstellmöglichkeiten (gespeichert im NVS):
- WLAN-Einstellungen (Scan, SSID, Passwort).
- Netzwerk-Einstellungen (Optional): Felder für feste IP-Adresse, Gateway und Subnetzmaske, um DHCP zu umgehen.
- Server-Einstellungen (IP/Hostname, Port).
- Geräte-ID (clientId).
- Mikrofon-Modus (Auswahl: "Immer aktiv" oder "Nur bei Tastendruck").
- Diagnose & Test-Sektion.
1. Wichtige Entwickler-Notizen
- Task-Management: Die Verwendung von FreeRTOS-Tasks ist für eine stabile Streaming-Anwendung unerlässlich, um blockierende Operationen zu vermeiden.
- Latenz-Management im Deep-Sleep-Modus: Die beschriebene Audio-Pufferung ist der Schlüssel für eine gute User Experience. Sie verbirgt die technische Verbindungs-Latenz vollständig vor dem Benutzer, sodass dieser sofort nach dem Tastendruck sprechen kann.
- Vollständige Übertragung: Es muss sichergestellt werden, dass der WebSocketClient nach dem Loslassen des Tasters die Sendeschleife erst dann beendet, wenn alle Puffer leer und alle Chunks übertragen sind.
- Power-Management: Der PowerManager ist die zentrale Komponente für den ereignisbasierten Modus und muss eng mit dem WebSocketClient und AudioManager zusammenarbeiten, um den richtigen Zeitpunkt für den Übergang in den Deep Sleep zu bestimmen.