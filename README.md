# M5Stack ATOM Echo Client

Ein stabiler und optimierter Echo-Client für den M5Stack ATOM Echo mit Hardware-Rauschen-Fix und verbesserter WLAN-Verbindung.

## 🚀 Version 0.9 - Stabilisierte Version

### ✨ Neue Features & Verbesserungen

- **Hardware-Rauschen behoben**: Korrigierte I2S-Pin-Konfiguration für M5Stack ATOM Echo
- **WLAN-Verbindung verbessert**: Automatischer Fallback auf AP-Modus bei Verbindungsfehlern
- **LED-Status korrigiert**: Richtige Farbanzeige basierend auf System-Status
- **Stack-Overflow behoben**: Erhöhte Task-Stack-Größen für Stabilität
- **EventManager deaktiviert**: Temporär deaktiviert für bessere WLAN-Stabilität

### 🔧 Technische Details

- **Platform**: ESP32 (M5Stack ATOM Echo)
- **Framework**: Arduino
- **Build System**: PlatformIO
- **Audio**: I2S Interface mit optimierter Puffer-Verwaltung
- **WLAN**: Dual-Modus (STA + AP) mit automatischem Fallback

## 📋 Hardware-Anforderungen

- M5Stack ATOM Echo
- USB-C Kabel
- WLAN-Netzwerk für Konfiguration

## 🛠️ Installation & Setup

### 1. Repository klonen
```bash
git clone https://github.com/yourusername/m5echoclient.git
cd m5echoclient/M5_Echo_Firmware
```

### 2. Dependencies installieren
```bash
pio install
```

### 3. Firmware flashen
```bash
pio run --target upload
```

### 4. Serial Monitor öffnen
```bash
pio device monitor
```

## 🔧 Konfiguration

### WLAN-Konfiguration
1. Verbinden Sie sich mit dem Access Point "M5_Echo_Setup"
2. Öffnen Sie http://192.168.4.1 im Browser
3. Geben Sie Ihre WLAN-Daten ein
4. Das Gerät verbindet sich automatisch

### LED-Status-Indikatoren
- **Blau blinkend**: WLAN-Verbindung wird aufgebaut
- **Grün**: Verbunden und bereit
- **Gelb**: Access Point Modus (Konfiguration)
- **Rot**: Fehler

## 📁 Projektstruktur

```
M5_Echo_Firmware/
├── src/
│   ├── main.cpp              # Hauptprogramm
│   ├── config.h              # Konfiguration
│   ├── AudioManager.cpp/h    # Audio-Verwaltung
│   ├── WifiManager.cpp/h     # WLAN-Management
│   ├── LedManager.cpp/h      # LED-Steuerung
│   ├── PowerManager.cpp/h    # Strom-Management
│   ├── WebSocketClient.cpp/h # WebSocket-Client
│   └── OtaManager.cpp/h      # OTA-Updates
├── platformio.ini           # PlatformIO Konfiguration
└── README.md               # Projekt-Dokumentation
```

## 🐛 Bekannte Probleme

- EventManager ist temporär deaktiviert (für WLAN-Stabilität)
- Einige Legacy-ADC-Warnungen (harmlos)

## 🔄 Changelog

### v0.9 (Aktuell)
- ✅ Hardware-Rauschen behoben
- ✅ WLAN-Verbindung stabilisiert
- ✅ LED-Status korrigiert
- ✅ Stack-Overflow behoben
- ✅ EventManager deaktiviert

## 🤝 Beitragen

1. Fork das Repository
2. Erstellen Sie einen Feature-Branch
3. Committen Sie Ihre Änderungen
4. Erstellen Sie einen Pull Request

## 📄 Lizenz

Dieses Projekt steht unter der MIT-Lizenz.

## 📞 Support

Bei Problemen oder Fragen:
1. Überprüfen Sie die Serial Monitor Ausgabe
2. Erstellen Sie ein Issue auf GitHub
3. Prüfen Sie die Hardware-Verbindungen

---

**Entwickelt für M5Stack ATOM Echo** 🎵
