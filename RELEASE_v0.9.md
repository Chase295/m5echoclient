# M5Stack ATOM Echo Client v0.9

## 🎉 Release Notes

### ✨ Neue Features & Verbesserungen

#### 🔧 Hardware-Rauschen behoben
- **I2S-Pin-Konfiguration korrigiert**: Verifizierte Pin-Zuordnung für M5Stack ATOM Echo
- **Verstärker-Steuerung optimiert**: GPIO 25 ausschließlich für Lautsprecher-Verstärker
- **Audio-Puffer-Verwaltung verbessert**: Separate Puffer für Audio und Stille

#### 🌐 WLAN-Verbindung stabilisiert
- **Automatischer AP-Fallback**: Bei Verbindungsfehlern wird automatisch der Access-Point-Modus gestartet
- **Verbesserte Fehlerbehandlung**: Bessere Behandlung von `4WAY_HANDSHAKE_TIMEOUT` Fehlern
- **Dual-Modus-Unterstützung**: STA + AP Modus für flexible Konfiguration

#### 💡 LED-Status korrigiert
- **Richtige Farbanzeige**: LED-Status basiert jetzt auf tatsächlichem System-Status
- **Dynamische Updates**: LED-Farben werden alle 5 Sekunden aktualisiert
- **Klare Status-Indikatoren**: 
  - Blau blinkend: WLAN-Verbindung wird aufgebaut
  - Grün: Verbunden und bereit
  - Gelb: Access Point Modus (Konfiguration)
  - Rot: Fehler

#### 🛡️ Stabilität verbessert
- **Stack-Overflow behoben**: Erhöhte Task-Stack-Größen (16KB statt 4KB)
- **EventManager deaktiviert**: Temporär deaktiviert für bessere WLAN-Stabilität
- **Memory-Management optimiert**: Reduzierte Memory-Fragmentation

### 🔧 Technische Details

#### Audio-System
```cpp
// Korrigierte I2S-Pin-Konfiguration
#define I2S_MIC_BCK_PIN      19
#define I2S_MIC_WS_PIN       33
#define I2S_MIC_DATA_PIN     23
#define I2S_SPEAKER_BCK_PIN  19  // Teilt sich mit Mikrofon
#define I2S_SPEAKER_WS_PIN   33  // Teilt sich mit Mikrofon
#define I2S_SPEAKER_DATA_PIN 22
#define SPEAKER_ENABLE_PIN   25  // Exklusiv für Verstärker
```

#### WLAN-System
- Automatischer Fallback auf AP-Modus bei Verbindungsfehlern
- Verbesserte Fehlerbehandlung für verschiedene WLAN-Fehler
- Konfigurations-Webserver unter http://192.168.4.1

#### Task-Management
- Erhöhte Stack-Größen für alle Audio-Tasks
- Optimierte Task-Prioritäten
- Verbesserte Memory-Verwaltung

### 📋 Installation

1. **Repository klonen**:
   ```bash
   git clone https://github.com/Chase295/m5echoclient.git
   cd m5echoclient/M5_Echo_Firmware
   ```

2. **Dependencies installieren**:
   ```bash
   pio install
   ```

3. **Firmware flashen**:
   ```bash
   pio run --target upload
   ```

4. **Serial Monitor öffnen**:
   ```bash
   pio device monitor
   ```

### 🔧 Konfiguration

#### WLAN-Konfiguration
1. Verbinden Sie sich mit dem Access Point "M5_Echo_Setup"
2. Öffnen Sie http://192.168.4.1 im Browser
3. Geben Sie Ihre WLAN-Daten ein
4. Das Gerät verbindet sich automatisch

#### LED-Status
- **Blau blinkend**: WLAN-Verbindung wird aufgebaut
- **Grün**: Verbunden und bereit
- **Gelb**: Access Point Modus (Konfiguration)
- **Rot**: Fehler

### 🐛 Bekannte Probleme

- EventManager ist temporär deaktiviert (für WLAN-Stabilität)
- Einige Legacy-ADC-Warnungen (harmlos, beeinträchtigt nicht die Funktionalität)

### 🔄 Changelog

#### v0.9 (Aktuell)
- ✅ Hardware-Rauschen behoben
- ✅ WLAN-Verbindung stabilisiert
- ✅ LED-Status korrigiert
- ✅ Stack-Overflow behoben
- ✅ EventManager deaktiviert
- ✅ Automatischer AP-Fallback implementiert
- ✅ Verbesserte Fehlerbehandlung

### 📁 Dateien

- `M5_Echo_Client_v0.9.zip`: Vollständiges Release-Paket
- `src/`: Quellcode-Dateien
- `platformio.ini`: PlatformIO-Konfiguration
- `README.md`: Projekt-Dokumentation
- `M5_ECHO_SPEZIFIKATION.md`: Technische Spezifikation

### 🤝 Support

Bei Problemen oder Fragen:
1. Überprüfen Sie die Serial Monitor Ausgabe
2. Erstellen Sie ein Issue auf GitHub
3. Prüfen Sie die Hardware-Verbindungen

---

**Entwickelt für M5Stack ATOM Echo** 🎵

**Release-Datum**: 2. September 2025  
**Version**: 0.9  
**Stabilität**: ✅ Stabil
