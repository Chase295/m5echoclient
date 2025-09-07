#include "WifiManager.h"
#include <ArduinoJson.h>
#include <ESPmDNS.h>
#include "LedManager.h"
// #include "EventManager.h"  // Temporär deaktiviert

// =============================================================================
// HTML-TEMPLATES
// =============================================================================

const char HTML_ADMIN_DASHBOARD_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>M5 Echo Dashboard</title>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: Arial, sans-serif; background: #f4f4f4; color: #333; }
        .container { max-width: 600px; margin: auto; background: white; padding: 20px; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }
        h1 { color: #0056b3; }
        .status { margin-bottom: 20px; }
        .status p { margin: 5px 0; }
        .actions button { display: block; width: 100%; padding: 10px; margin-bottom: 10px; border: none; border-radius: 5px; background: #007bff; color: white; font-size: 16px; cursor: pointer; }
        .actions button:hover { background: #0056b3; }
        .config-item { margin-bottom: 15px; padding: 10px; border: 1px solid #eee; border-radius: 5px; background: #f9f9f9; }
        .config-item label { display: block; margin-bottom: 5px; font-weight: bold; }
        .config-item input[type="color"] { margin-right: 10px; }
        .config-item select { margin-left: 10px; padding: 5px; border-radius: 3px; border: 1px solid #ddd; }
        .config-item input[type="range"] { width: 200px; margin-right: 10px; }
        #logs { margin-top: 20px; border: 1px solid #ddd; background: #fafafa; height: 150px; overflow-y: scroll; text-align: left; padding: 10px; white-space: pre-wrap; font-family: monospace; }
    </style>
</head>
<body>
    <div class="container">
        <h1>M5 Echo Admin Dashboard</h1>
        <div class="status">
            <p><strong>IP-Adresse:</strong> <span id="ip_address">%IP_ADDRESS%</span></p>
            <p><strong>WLAN-Signal (RSSI):</strong> <span id="rssi">...</span></p>
            <p><strong>WebSocket Status:</strong> <span id="ws_status">...</span></p>
        </div>
        <div class="actions">
            <button id="testToneBtn">Test-Ton abspielen</button>
            <button id="rebootBtn">Gerät neustarten</button>
        </div>
        
        <h3>System-Konfiguration</h3>
        
        <fieldset>
            <legend>Mikrofon-Modus</legend>
            <div class="config-item">
                <label>
                    <input type="radio" name="mic_mode" value="always_on">
                    Dauerbetrieb (Mikrofon immer aktiv bei Verbindung)
                </label>
                <br>
                <label>
                    <input type="radio" name="mic_mode" value="on_button_press">
                    Nur bei Knopfdruck (Energiesparmodus)
                </label>
            </div>
        </fieldset>
        
        <fieldset>
            <legend>Live-Diagnose</legend>
            <div class="config-item">
                <label>
                    <input type="checkbox" id="micDebugSwitch">
                    Mikrofon-Lautstärke im Log anzeigen (1/sek)
                </label>
            </div>
        </fieldset>
        
        <h3>Umfassende LED-Konfiguration</h3>
        <form id="ledConfigForm">
            <fieldset>
                <legend>Booten (BOOTING)</legend>
                <label>Aktiv: <input type="checkbox" id="active_BOOTING"></label>
                <label>Farbe: <input type="color" id="color_BOOTING"></label>
                <label>Helligkeit: <input type="range" id="brightness_BOOTING" min="0" max="255"></label>
                <label>Effekt: 
                    <select id="effect_BOOTING" class="effect-selector">
                        <option value="STATIC">Statisch</option>
                        <option value="PULSING">Pulsieren</option>
                        <option value="BREATHING">Atmen</option>
                    </select>
                </label>
                <label>Geschwindigkeit: <input type="range" id="speed_BOOTING" min="1" max="100"></label>
                
                <div id="breathing_options_BOOTING" class="breathing-options" style="display: none; margin-top: 10px; padding-top: 10px; border-top: 1px solid #eee;">
                    <label>Min Helligkeit: <input type="range" id="min_brightness_BOOTING" min="0" max="255" value="0"></label><br>
                    <label>Max Helligkeit: <input type="range" id="max_brightness_BOOTING" min="0" max="255" value="255"></label><br>
                    <label>Regenbogen-Modus: <input type="checkbox" id="rainbow_BOOTING"></label>
                </div>
            </fieldset>

            <fieldset>
                <legend>Kein WLAN (NO_WIFI)</legend>
                <label>Aktiv: <input type="checkbox" id="active_NO_WIFI"></label>
                <label>Farbe: <input type="color" id="color_NO_WIFI"></label>
                <label>Helligkeit: <input type="range" id="brightness_NO_WIFI" min="0" max="255"></label>
                <label>Effekt: 
                    <select id="effect_NO_WIFI" class="effect-selector">
                        <option value="STATIC">Statisch</option>
                        <option value="PULSING">Pulsieren</option>
                        <option value="BREATHING">Atmen</option>
                    </select>
                </label>
                <label>Geschwindigkeit: <input type="range" id="speed_NO_WIFI" min="1" max="100"></label>
                
                <div id="breathing_options_NO_WIFI" class="breathing-options" style="display: none; margin-top: 10px; padding-top: 10px; border-top: 1px solid #eee;">
                    <label>Min Helligkeit: <input type="range" id="min_brightness_NO_WIFI" min="0" max="255" value="0"></label><br>
                    <label>Max Helligkeit: <input type="range" id="max_brightness_NO_WIFI" min="0" max="255" value="255"></label><br>
                    <label>Regenbogen-Modus: <input type="checkbox" id="rainbow_NO_WIFI"></label>
                </div>
            </fieldset>

            <fieldset>
                <legend>Kein WebSocket (NO_WEBSOCKET)</legend>
                <label>Aktiv: <input type="checkbox" id="active_NO_WEBSOCKET"></label>
                <label>Farbe: <input type="color" id="color_NO_WEBSOCKET"></label>
                <label>Helligkeit: <input type="range" id="brightness_NO_WEBSOCKET" min="0" max="255"></label>
                <label>Effekt: 
                    <select id="effect_NO_WEBSOCKET" class="effect-selector">
                        <option value="STATIC">Statisch</option>
                        <option value="PULSING">Pulsieren</option>
                        <option value="BREATHING">Atmen</option>
                    </select>
                </label>
                <label>Geschwindigkeit: <input type="range" id="speed_NO_WEBSOCKET" min="1" max="100"></label>
                
                <div id="breathing_options_NO_WEBSOCKET" class="breathing-options" style="display: none; margin-top: 10px; padding-top: 10px; border-top: 1px solid #eee;">
                    <label>Min Helligkeit: <input type="range" id="min_brightness_NO_WEBSOCKET" min="0" max="255" value="0"></label><br>
                    <label>Max Helligkeit: <input type="range" id="max_brightness_NO_WEBSOCKET" min="0" max="255" value="255"></label><br>
                    <label>Regenbogen-Modus: <input type="checkbox" id="rainbow_NO_WEBSOCKET"></label>
                </div>
            </fieldset>

            <fieldset>
                <legend>Mikrofon aktiv (LISTENING)</legend>
                <label>Aktiv: <input type="checkbox" id="active_LISTENING"></label>
                <label>Farbe: <input type="color" id="color_LISTENING"></label>
                <label>Helligkeit: <input type="range" id="brightness_LISTENING" min="0" max="255"></label>
                <label>Effekt: 
                    <select id="effect_LISTENING" class="effect-selector">
                        <option value="STATIC">Statisch</option>
                        <option value="PULSING">Pulsieren</option>
                        <option value="BREATHING">Atmen</option>
                    </select>
                </label>
                <label>Geschwindigkeit: <input type="range" id="speed_LISTENING" min="1" max="100"></label>
                
                <div id="breathing_options_LISTENING" class="breathing-options" style="display: none; margin-top: 10px; padding-top: 10px; border-top: 1px solid #eee;">
                    <label>Min Helligkeit: <input type="range" id="min_brightness_LISTENING" min="0" max="255" value="0"></label><br>
                    <label>Max Helligkeit: <input type="range" id="max_brightness_LISTENING" min="0" max="255" value="255"></label><br>
                    <label>Regenbogen-Modus: <input type="checkbox" id="rainbow_LISTENING"></label>
                </div>
            </fieldset>

            <fieldset>
                <legend>Audio-Wiedergabe (PLAYING)</legend>
                <label>Aktiv: <input type="checkbox" id="active_PLAYING"></label>
                <label>Farbe: <input type="color" id="color_PLAYING"></label>
                <label>Helligkeit: <input type="range" id="brightness_PLAYING" min="0" max="255"></label>
                <label>Effekt: 
                    <select id="effect_PLAYING" class="effect-selector">
                        <option value="STATIC">Statisch</option>
                        <option value="PULSING">Pulsieren</option>
                        <option value="BREATHING">Atmen</option>
                    </select>
                </label>
                <label>Geschwindigkeit: <input type="range" id="speed_PLAYING" min="1" max="100"></label>
                
                <div id="breathing_options_PLAYING" class="breathing-options" style="display: none; margin-top: 10px; padding-top: 10px; border-top: 1px solid #eee;">
                    <label>Min Helligkeit: <input type="range" id="min_brightness_PLAYING" min="0" max="255" value="0"></label><br>
                    <label>Max Helligkeit: <input type="range" id="max_brightness_PLAYING" min="0" max="255" value="255"></label><br>
                    <label>Regenbogen-Modus: <input type="checkbox" id="rainbow_PLAYING"></label>
                </div>
            </fieldset>

            <fieldset>
                <legend>Ruhezustand (IDLE)</legend>
                <label>Aktiv: <input type="checkbox" id="active_IDLE"></label>
                <label>Farbe: <input type="color" id="color_IDLE"></label>
                <label>Helligkeit: <input type="range" id="brightness_IDLE" min="0" max="255"></label>
                <label>Effekt: 
                    <select id="effect_IDLE" class="effect-selector">
                        <option value="STATIC">Statisch</option>
                        <option value="PULSING">Pulsieren</option>
                        <option value="BREATHING">Atmen</option>
                    </select>
                </label>
                <label>Geschwindigkeit: <input type="range" id="speed_IDLE" min="1" max="100"></label>
                
                <div id="breathing_options_IDLE" class="breathing-options" style="display: none; margin-top: 10px; padding-top: 10px; border-top: 1px solid #eee;">
                    <label>Min Helligkeit: <input type="range" id="min_brightness_IDLE" min="0" max="255" value="0"></label><br>
                    <label>Max Helligkeit: <input type="range" id="max_brightness_IDLE" min="0" max="255" value="255"></label><br>
                    <label>Regenbogen-Modus: <input type="checkbox" id="rainbow_IDLE"></label>
                </div>
            </fieldset>

            <fieldset>
                <legend>Denken (THINKING)</legend>
                <label>Aktiv: <input type="checkbox" id="active_THINKING"></label>
                <label>Farbe: <input type="color" id="color_THINKING"></label>
                <label>Helligkeit: <input type="range" id="brightness_THINKING" min="0" max="255"></label>
                <label>Effekt: 
                    <select id="effect_THINKING" class="effect-selector">
                        <option value="STATIC">Statisch</option>
                        <option value="PULSING">Pulsieren</option>
                        <option value="BREATHING">Atmen</option>
                    </select>
                </label>
                <label>Geschwindigkeit: <input type="range" id="speed_THINKING" min="1" max="100"></label>
                
                <div id="breathing_options_THINKING" class="breathing-options" style="display: none; margin-top: 10px; padding-top: 10px; border-top: 1px solid #eee;">
                    <label>Min Helligkeit: <input type="range" id="min_brightness_THINKING" min="0" max="255" value="0"></label><br>
                    <label>Max Helligkeit: <input type="range" id="max_brightness_THINKING" min="0" max="255" value="255"></label><br>
                    <label>Regenbogen-Modus: <input type="checkbox" id="rainbow_THINKING"></label>
                </div>
            </fieldset>

            <fieldset>
                <legend>OTA Update (OTA_UPDATE)</legend>
                <label>Aktiv: <input type="checkbox" id="active_OTA_UPDATE"></label>
                <label>Farbe: <input type="color" id="color_OTA_UPDATE"></label>
                <label>Helligkeit: <input type="range" id="brightness_OTA_UPDATE" min="0" max="255"></label>
                <label>Effekt: 
                    <select id="effect_OTA_UPDATE" class="effect-selector">
                        <option value="STATIC">Statisch</option>
                        <option value="PULSING">Pulsieren</option>
                        <option value="BREATHING">Atmen</option>
                    </select>
                </label>
                <label>Geschwindigkeit: <input type="range" id="speed_OTA_UPDATE" min="1" max="100"></label>
                
                <div id="breathing_options_OTA_UPDATE" class="breathing-options" style="display: none; margin-top: 10px; padding-top: 10px; border-top: 1px solid #eee;">
                    <label>Min Helligkeit: <input type="range" id="min_brightness_OTA_UPDATE" min="0" max="255" value="0"></label><br>
                    <label>Max Helligkeit: <input type="range" id="max_brightness_OTA_UPDATE" min="0" max="255" value="255"></label><br>
                    <label>Regenbogen-Modus: <input type="checkbox" id="rainbow_OTA_UPDATE"></label>
                </div>
            </fieldset>

            <fieldset>
                <legend>Fehler (ERROR)</legend>
                <label>Aktiv: <input type="checkbox" id="active_ERROR"></label>
                <label>Farbe: <input type="color" id="color_ERROR"></label>
                <label>Helligkeit: <input type="range" id="brightness_ERROR" min="0" max="255"></label>
                <label>Effekt: 
                    <select id="effect_ERROR" class="effect-selector">
                        <option value="STATIC">Statisch</option>
                        <option value="PULSING">Pulsieren</option>
                        <option value="BREATHING">Atmen</option>
                    </select>
                </label>
                <label>Geschwindigkeit: <input type="range" id="speed_ERROR" min="1" max="100"></label>
                
                <div id="breathing_options_ERROR" class="breathing-options" style="display: none; margin-top: 10px; padding-top: 10px; border-top: 1px solid #eee;">
                    <label>Min Helligkeit: <input type="range" id="min_brightness_ERROR" min="0" max="255" value="0"></label><br>
                    <label>Max Helligkeit: <input type="range" id="max_brightness_ERROR" min="0" max="255" value="255"></label><br>
                    <label>Regenbogen-Modus: <input type="checkbox" id="rainbow_ERROR"></label>
                </div>
            </fieldset>



            <button type="submit">LED-Konfiguration speichern</button>
        </form>
        
        <h3>Live Logs</h3>
        <div id="logs">Verbinde mit WebSocket für Logs...</div>
    </div>

    <script>
        // Update status periodically
        setInterval(function() {
            fetch('/status.json')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('rssi').textContent = data.rssi + ' dBm';
                    document.getElementById('ws_status').textContent = data.ws_status;
                })
                .catch(error => console.error('Error fetching status:', error));
        }, 5000);

        // Button actions
        document.getElementById('testToneBtn').addEventListener('click', () => fetch('/test-tone', { method: 'POST' }));
        document.getElementById('rebootBtn').addEventListener('click', () => {
            if (confirm('Gerät wirklich neustarten?')) {
                fetch('/reboot', { method: 'POST' });
            }
        });
        

        
        // LED Config form handler
        document.getElementById('ledConfigForm').addEventListener('submit', function(event) {
            event.preventDefault();
            
            console.log('[Web UI] Speichere LED-Konfiguration...');
            
            const config = {};
            const states = ['BOOTING', 'NO_WIFI', 'NO_WEBSOCKET', 'LISTENING', 'PLAYING', 
                           'IDLE', 'THINKING', 'OTA_UPDATE', 'ERROR'];
            
            let validConfig = true;
            states.forEach(state => {
                const activeElement = document.getElementById('active_' + state);
                const colorElement = document.getElementById('color_' + state);
                const brightnessElement = document.getElementById('brightness_' + state);
                const effectElement = document.getElementById('effect_' + state);
                const speedElement = document.getElementById('speed_' + state);
                
                if (activeElement && colorElement && brightnessElement && effectElement && speedElement) {
                    config[state] = {
                        active: activeElement.checked,
                        color: colorElement.value.substring(1), // # entfernen
                        brightness: parseInt(brightnessElement.value),
                        effect: effectElement.value,
                        speed: parseInt(speedElement.value),
                        minBrightness: parseInt(document.getElementById('min_brightness_' + state).value) || 0,
                        maxBrightness: parseInt(document.getElementById('max_brightness_' + state).value) || 255,
                        rainbowMode: document.getElementById('rainbow_' + state).checked || false
                    };
                } else {
                    console.error('[Web UI] Fehlende Elemente für Zustand:', state);
                    validConfig = false;
                }
            });
            
            if (!validConfig) {
                alert('Fehler: Nicht alle Konfigurationselemente gefunden. Bitte Seite neu laden.');
                return;
            }
            
            console.log('[Web UI] Konfiguration erstellt:', config);
            
            fetch('/save-led-config', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify(config)
            })
            .then(response => {
                if (!response.ok) {
                    throw new Error('HTTP ' + response.status);
                }
                return response.text();
            })
            .then(result => {
                console.log('[Web UI] Speichern erfolgreich:', result);
                alert('Gespeichert! Die neuen Einstellungen werden sofort aktiv.');
            })
            .catch(error => {
                console.error('[Web UI] Fehler beim Speichern:', error);
                alert('Fehler beim Speichern der Konfiguration. Bitte erneut versuchen.');
            });
        });
        

        

        
        // Konfiguration beim Laden der Seite abrufen
        document.addEventListener('DOMContentLoaded', function() {
            console.log('[Web UI] Lade System-Konfiguration...');
            fetch('/get-system-config')
                .then(response => {
                    if (!response.ok) {
                        throw new Error('HTTP ' + response.status);
                    }
                    return response.json();
                })
                .then(data => {
                    console.log('[Web UI] System-Konfiguration empfangen:', data);
                    
                    // Mikrofon-Modus setzen
                    if (data.mic_mode) {
                        const micRadio = document.querySelector(`input[name="mic_mode"][value="${data.mic_mode}"]`);
                        if (micRadio) {
                            micRadio.checked = true;
                            console.log('[Web UI] Mikrofon-Modus gesetzt:', data.mic_mode);
                        }
                    }
                    
                    // LED-Konfiguration verarbeiten
                    const ledData = data.led_config || data;
                    
                                // Funktion, um die speziellen Optionen für einen Zustand anzuzeigen/verstecken
            function updateBreathingOptionsVisibility(stateKey) {
                const effectSelector = document.getElementById('effect_' + stateKey);
                const breathingOptions = document.getElementById('breathing_options_' + stateKey);
                if (effectSelector.value === 'BREATHING') {
                    breathingOptions.style.display = 'block';
                } else {
                    breathingOptions.style.display = 'none';
                }
            }
            
                                // Alle Zustände durchgehen und Formularfelder füllen
                    const states = ['BOOTING', 'NO_WIFI', 'NO_WEBSOCKET', 'LISTENING', 'PLAYING', 
                                   'IDLE', 'THINKING', 'OTA_UPDATE', 'ERROR'];
                    
                    // Event-Listener für alle Effekt-Dropdowns hinzufügen
                    states.forEach(key => {
                        const selector = document.getElementById('effect_' + key);
                        if(selector) {
                            selector.addEventListener('change', () => updateBreathingOptionsVisibility(key));
                        }
                    });
                    
                    let loadedCount = 0;
                    states.forEach(state => {
                        if (ledData[state]) {
                            const config = ledData[state];
                            
                            // Checkbox für Aktiv/Inaktiv
                            const activeCheckbox = document.getElementById('active_' + state);
                            if (activeCheckbox) {
                                activeCheckbox.checked = config.active;
                                loadedCount++;
                            }
                            
                            // Farbwähler
                            const colorInput = document.getElementById('color_' + state);
                            if (colorInput) {
                                colorInput.value = '#' + config.color;
                                loadedCount++;
                            }
                            
                            // Helligkeit
                            const brightnessInput = document.getElementById('brightness_' + state);
                            if (brightnessInput) {
                                brightnessInput.value = config.brightness;
                                loadedCount++;
                            }
                            
                            // Effekt
                            const effectSelect = document.getElementById('effect_' + state);
                            if (effectSelect) {
                                effectSelect.value = config.effect;
                                loadedCount++;
                            }
                            
                            // Geschwindigkeit
                            const speedInput = document.getElementById('speed_' + state);
                            if (speedInput) {
                                speedInput.value = config.speed;
                                loadedCount++;
                            }
                            
                            // Neue Felder für Atmen-Effekt
                            const minBrightnessInput = document.getElementById('min_brightness_' + state);
                            if (minBrightnessInput) {
                                minBrightnessInput.value = config.minBrightness || 0;
                                loadedCount++;
                            }
                            
                            const maxBrightnessInput = document.getElementById('max_brightness_' + state);
                            if (maxBrightnessInput) {
                                maxBrightnessInput.value = config.maxBrightness || 255;
                                loadedCount++;
                            }
                            
                            const rainbowCheckbox = document.getElementById('rainbow_' + state);
                            if (rainbowCheckbox) {
                                rainbowCheckbox.checked = config.rainbowMode || false;
                                loadedCount++;
                            }
                            
                            // Sichtbarkeit der Atmen-Optionen initial setzen
                            updateBreathingOptionsVisibility(state);
                        } else {
                            console.warn('[Web UI] Keine Konfiguration für Zustand:', state);
                        }
                    });
                    
                    console.log(`[Web UI] ${loadedCount} Formularfelder erfolgreich geladen.`);
                })
                .catch(error => {
                    console.error('[Web UI] Fehler beim Laden der Konfiguration:', error);
                    alert('Fehler beim Laden der LED-Konfiguration. Bitte Seite neu laden.');
                });
        });
        
        // Mikrofon-Modus Event-Listener
        document.querySelectorAll('input[name="mic_mode"]').forEach(radio => {
            radio.addEventListener('change', function() {
                const newMode = this.value;
                console.log('[Web UI] Mikrofon-Modus geändert:', newMode);
                
                fetch('/set-mic-mode', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ mode: newMode })
                })
                .then(response => {
                    if (!response.ok) {
                        throw new Error('HTTP ' + response.status);
                    }
                    return response.text();
                })
                .then(result => {
                    console.log('[Web UI] Mikrofon-Modus gespeichert:', result);
                    alert('Mikrofon-Modus gespeichert! Ein Neustart ist erforderlich, um die Änderung vollständig zu aktivieren.');
                })
                .catch(error => {
                    console.error('[Web UI] Fehler beim Speichern des Mikrofon-Modus:', error);
                    alert('Fehler beim Speichern des Mikrofon-Modus. Bitte erneut versuchen.');
                });
            });
        });
        
        // Mikrofon-Debug-Modus Event-Listener
        document.getElementById('micDebugSwitch').addEventListener('change', function() {
            console.log('[Web UI] Mikrofon-Debug-Modus geändert:', this.checked);
            
            fetch('/set-mic-debug', {
                method: 'POST',
                headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                body: 'active=' + this.checked
            })
            .then(response => {
                if (!response.ok) {
                    throw new Error('HTTP ' + response.status);
                }
                return response.text();
            })
            .then(result => {
                console.log('[Web UI] Mikrofon-Debug-Modus gespeichert:', result);
            })
            .catch(error => {
                console.error('[Web UI] Fehler beim Speichern des Mikrofon-Debug-Modus:', error);
            });
        });
        
        // --- NEUE "Short Polling"-Logik für Live-Logs ---
        const logContainer = document.getElementById('logs');
        logContainer.innerHTML = 'Warte auf erste Log-Nachrichten...\n';

        setInterval(function() {
            fetch('/get-logs')
                .then(response => response.text())
                .then(text => {
                    if (text.length > 0) {
                        logContainer.innerHTML += text; // Neue Logs hinzufügen
                        logContainer.scrollTop = logContainer.scrollHeight; // Auto-scroll
                    }
                })
                .catch(error => console.error('Fehler beim Abrufen der Logs:', error));
        }, 2000); // Alle 2000ms (2 Sekunden) nachfragen
    </script>
</body>
</html>
)rawliteral";

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
    serverInitialized = false;  // Webserver-Status initialisieren
    
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
    if (webServer) {
        webServer->handleClient();
        // DNS-Server nur im AP-Modus verarbeiten
        if (isAccessPointMode() && dnsServer) {
            dnsServer->processNextRequest();
        }
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
    Serial.println("--- FINALE ÜBERGABE AN WIFI-CHIP ---");
    Serial.printf("SSID: '%s'\n", ssid.c_str());
    Serial.printf("Passwort: '%s***%s' (Länge: %d)\n", 
                  password.substring(0, 1).c_str(), 
                  password.substring(password.length() - 1).c_str(),
                  password.length());
    Serial.println("------------------------------------");
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
        
        // ===== NETZWERK-DIAGNOSE NACH ERFOLGREICHER VERBINDUNG =====
        Serial.println("\n[Netzwerk] Erfolgreich mit WLAN verbunden!");
        Serial.print("[Netzwerk] IP-Adresse erhalten: ");
        Serial.println(WiFi.localIP());
        Serial.printf("[Netzwerk] Gateway: %s\n", WiFi.gatewayIP().toString().c_str());
        Serial.printf("[Netzwerk] Subnetz-Maske: %s\n", WiFi.subnetMask().toString().c_str());
        Serial.printf("[Netzwerk] DNS-Server: %s\n", WiFi.dnsIP().toString().c_str());
        Serial.printf("[Netzwerk] MAC-Adresse: %s\n", WiFi.macAddress().c_str());
        Serial.printf("[Netzwerk] RSSI: %d dBm\n", WiFi.RSSI());
        
        // Webserver im Client-Modus starten
        Serial.println("[Diagnose] Webserver im Client-Modus wird gestartet...");
        startWebServer();  // Zentrale Webserver-Start-Methode verwenden
        
        // mDNS-Dienst starten
        Serial.println("[Netzwerk] Starte mDNS-Dienst...");
        if (MDNS.begin("m5echo")) {
            Serial.println("[Netzwerk] mDNS-Responder gestartet. Gerät sollte unter http://m5echo.local erreichbar sein.");
            
            // mDNS-Dienste hinzufügen
            MDNS.addService("http", "tcp", 80);
            MDNS.addService("ws", "tcp", 8080);
            
            Serial.println("[Netzwerk] mDNS-Dienste registriert: HTTP (Port 80), WebSocket (Port 8080)");
        } else {
            Serial.println("[Netzwerk] Fehler beim Starten des mDNS-Responders!");
        }
        
        Serial.println("==========================================");
        return true;
    } else {
        currentStatus = WifiStatus::ERROR;
        Serial.printf("\nWifiManager: Verbindung fehlgeschlagen! WiFi Status-Code: %d\n", WiFi.status());
        
        // Detaillierte Status-Code-Interpretation
        switch (WiFi.status()) {
            case WL_IDLE_STATUS:
                Serial.println("[WiFi Error] WL_IDLE_STATUS - WiFi ist im Leerlauf");
                break;
            case WL_NO_SSID_AVAIL:
                Serial.println("[WiFi Error] WL_NO_SSID_AVAIL - SSID nicht gefunden");
                break;
            case WL_SCAN_COMPLETED:
                Serial.println("[WiFi Error] WL_SCAN_COMPLETED - Scan abgeschlossen");
                break;
            case WL_CONNECTED:
                Serial.println("[WiFi Error] WL_CONNECTED - Verbunden (unerwartet)");
                break;
            case WL_CONNECT_FAILED:
                Serial.println("[WiFi Error] WL_CONNECT_FAILED - Verbindung fehlgeschlagen");
                break;
            case WL_CONNECTION_LOST:
                Serial.println("[WiFi Error] WL_CONNECTION_LOST - Verbindung verloren");
                break;
            case WL_DISCONNECTED:
                Serial.println("[WiFi Error] WL_DISCONNECTED - Getrennt");
                break;
            default:
                Serial.printf("[WiFi Error] Unbekannter Status-Code: %d\n", WiFi.status());
                break;
        }
        
        return false;
    }
}

bool WifiManager::connect() {
    Serial.println("=== CONNECT MIT GESPEICHERTEN DATEN START ===");
    
    if (config.ssid.length() == 0) {
        Serial.println("WifiManager: Keine SSID konfiguriert - Starte AP-Modus");
        startAccessPointMode();
        return false;
    }
    
    Serial.printf("WifiManager: Verbinde mit %s...\n", config.ssid.c_str());
    Serial.printf("[Connect] Verwende gespeicherte SSID: '%s'\n", config.ssid.c_str());
    Serial.printf("[Connect] Verwende gespeichertes Passwort: '%s***%s' (Länge: %d)\n", 
                  config.password.substring(0, 1).c_str(), 
                  config.password.substring(config.password.length() - 1).c_str(),
                  config.password.length());
    
    bool success = connect(config.ssid, config.password);
    
    if (success) {
        // Webserver auch im Client-Modus starten
        if (!serverInitialized) {
            startWebServer();
        }
        currentStatus = WifiStatus::CONNECTED;
        Serial.println("WifiManager: Verbindung erfolgreich - Webserver gestartet");
    } else {
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
    if (webServer) {
        webServer->handleClient();
        // DNS-Server nur im AP-Modus verarbeiten
        if (isAccessPointMode() && dnsServer) {
            dnsServer->processNextRequest();
        }
    }
}

bool WifiManager::isWebServerActive() const {
    return webServer != nullptr && serverInitialized;
}

// =============================================================================
// PRIVATE METHODEN
// =============================================================================

void WifiManager::startAccessPoint() {
    // Webserver zurücksetzen für AP-Modus
    if (webServer) {
        delete webServer;
        webServer = nullptr;
        serverInitialized = false;
    }
    
    // Neue Webserver-Instanz erstellen und starten
    startWebServer();
    
    // DNS-Server für Captive Portal
    dnsServer->start(53, "*", IPAddress(192, 168, 4, 1));
    
    Serial.println("WifiManager: Web-Server im AP-Modus gestartet");
}

void WifiManager::stopAccessPoint() {
    if (webServer) {
        webServer->close();
    }
    if (dnsServer) {
        dnsServer->stop();
    }
}

void WifiManager::handleClient() {
    if (webServer) { // Sicherstellen, dass der Server existiert
        webServer->handleClient();
    }
}

void WifiManager::startWebServer() {
    // Schutzklausel, falls schon initialisiert
    if (serverInitialized && webServer) {
        Serial.println("[Diagnose] Webserver ist bereits initialisiert und läuft.");
        return;
    }

    // Webserver erstellen, falls noch nicht vorhanden
    if (!webServer) {
        webServer = new WebServer(80);
        Serial.println("[Diagnose] Webserver-Instanz erstellt.");
    }

    Serial.println("[Diagnose] Registriere Webserver-Handler...");
    
    // Hauptseite (Konfigurationsformular)
    webServer->on("/", HTTP_GET, [this]() { handleRoot(); });
    
    // Konfiguration speichern
    webServer->on("/save", HTTP_POST, [this]() { handleSave(); });
    
    // WLAN-Scan
    webServer->on("/scan", HTTP_GET, [this]() { handleScan(); });
    
    // Audio-Test
    webServer->on("/test-audio", HTTP_POST, [this]() { handleTestAudio(); });
    webServer->on("/test-tone", HTTP_POST, [this]() { handleTestAudio(); }); // Alternative Route
    
    // Neustart-Endpunkt
    webServer->on("/reboot", HTTP_POST, [this]() { handleReboot(); });
    
    // LED-Konfiguration speichern
    webServer->on("/save-led-config", HTTP_POST, [this]() { handleSaveLedConfig(); });
    
    // LED-Konfiguration laden
    webServer->on("/get-led-config", HTTP_GET, [this]() { handleGetLedConfig(); });
    
    // System-Konfiguration
    webServer->on("/get-system-config", HTTP_GET, [this]() { handleGetSystemConfig(); });
    webServer->on("/set-mic-mode", HTTP_POST, [this]() { handleSetMicMode(); });
    webServer->on("/set-mic-debug", HTTP_POST, [this]() { handleSetMicDebug(); });
    
    // LED Override-Modus setzen

    
    // Server-Sent Events für Live-Logs
    webServer->on("/events", HTTP_GET, [this]() { handleSSE(); });
    
    // Short Polling für Live-Logs
    webServer->on("/get-logs", HTTP_GET, [this]() { handleGetLogs(); });
    
    // Neue Diagnose-Endpunkte
    webServer->on("/ping", HTTP_GET, [this]() {
        this->webServer->send(200, "text/plain", "Pong! M5Stack ATOM Echo ist online.");
    });
    
    // Erweiterter Status-Endpunkt für das Dashboard
    webServer->on("/status.json", HTTP_GET, [this]() { handleStatusJson(); });
    
    // Legacy Status-Endpunkt (für Kompatibilität)
    webServer->on("/status", HTTP_GET, [this]() {
        String status = "{\"connected\":" + String(WiFi.status() == WL_CONNECTED ? "true" : "false") + 
                       ",\"ip\":\"" + WiFi.localIP().toString() + 
                       "\",\"ssid\":\"" + WiFi.SSID() + 
                       "\",\"rssi\":" + String(WiFi.RSSI()) + 
                       ",\"uptime\":" + String(millis()) + "}";
        this->webServer->send(200, "application/json", status);
    });
    
    // 404-Handler
    webServer->onNotFound([this]() { handleNotFound(); });
    
    Serial.println("[Diagnose] Alle Handler registriert.");

    Serial.println("[Diagnose] Starte Webserver...");
    webServer->begin();
    serverInitialized = true;
    Serial.println("[Diagnose] Webserver gestartet.");
}

void WifiManager::setupWebServer() {
    // Diese Methode wird jetzt durch startWebServer() ersetzt
    startWebServer();
}

void WifiManager::handleRoot() {
    Serial.println("[Web Server] Anfrage für / empfangen.");
    if (WiFi.getMode() == WIFI_AP) {
        // --- GERÄT IST IM SETUP-MODUS ---
        Serial.println("[Web Server] Liefere aus: SETUP-SEITE");
        // Sende die bestehende HTML-Seite für das Setup
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
    } else {
        // --- GERÄT IST IM NORMALEN CLIENT-MODUS ---
        Serial.println("[Web Server] Liefere aus: ADMIN-DASHBOARD");
        // Sende die neue Platzhalter-Seite für das Admin-Dashboard
        String page = HTML_ADMIN_DASHBOARD_PAGE;
        // Ersetze optional Platzhalter mit echten Werten
        page.replace("%IP_ADDRESS%", WiFi.localIP().toString());
        webServer->send(200, "text/html", page);
    }
}

void WifiManager::handleSave() {
    Serial.println("=== WEBSERVER HANDLER START ===");
    Serial.printf("[Webserver] Daten empfangen. Argumente: %d\n", webServer->args());
    
    // Alle Argumente auflisten
    for (int i = 0; i < webServer->args(); i++) {
        Serial.printf("[Webserver] Argument %d: '%s' = '%s'\n", i, webServer->argName(i).c_str(), webServer->arg(i).c_str());
    }
    
    // Prüfen, ob der Request Body (als 'plain' Argument) vorhanden ist
    if (webServer->hasArg("plain")) {
        String body = webServer->arg("plain");
        Serial.printf("[Webserver] JSON-Body empfangen: %s\n", body.c_str());

        // Ein JsonDocument erstellen, um die Daten sicher zu parsen.
        // Die Größe an die erwartete JSON-Größe anpassen.
        StaticJsonDocument<256> doc;
        
        // Den JSON-Body deserialisieren
        DeserializationError error = deserializeJson(doc, body);
        
        // Auf Fehler prüfen
        if (error) {
            Serial.print(F("[Webserver] deserializeJson() fehlgeschlagen: "));
            Serial.println(error.c_str());
            webServer->send(400, "text/plain", "Invalid JSON");
            return;
        }
        
        // Sicher auf die Werte zugreifen
        if (doc.containsKey("ssid") && doc.containsKey("password")) {
            String ssid = doc["ssid"];
            String password = doc["password"];
            
            Serial.printf("[Webserver] SSID aus JSON extrahiert: '%s'\n", ssid.c_str());
            Serial.printf("[Webserver] Passwort aus JSON extrahiert: '...' (Länge: %d)\n", password.length());
            
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
            Serial.println("[Webserver] JSON fehlen 'ssid' oder 'password' Schlüssel.");
            webServer->send(400, "text/plain", "Missing fields");
            return;
        }
    } else {
        // Kein JSON-Body vorhanden, versuche Form-Daten
        Serial.println("[Webserver] Kein 'plain' Body im Request gefunden, versuche Form-Daten.");
        
        // Fallback für Form-Daten
        Serial.println("[Webserver] Verarbeite Form-Daten (Fallback)");
        String ssid = webServer->hasArg("ssid") ? webServer->arg("ssid") : "";
        String password = webServer->hasArg("password") ? webServer->arg("password") : "";
        
        Serial.printf("[Webserver] SSID aus Formular extrahiert: '%s'\n", ssid.c_str());
        Serial.printf("[Webserver] Passwort aus Formular extrahiert: '%s***%s' (Länge: %d)\n", 
                      password.substring(0, 1).c_str(), 
                      password.substring(password.length() - 1).c_str(), 
                      password.length());
        
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

void WifiManager::handleReboot() {
    Serial.println("[Web Server] Neustart-Anfrage empfangen");
    webServer->send(200, "application/json", "{\"success\":true,\"message\":\"Gerät wird neu gestartet\"}");
    
    // Kurze Verzögerung für die Antwort
    delay(1000);
    
    // Gerät neu starten
    ESP.restart();
}

void WifiManager::handleStatusJson() {
    Serial.println("[Web Server] Status-Anfrage empfangen");
    
    // WebSocket-Status ermitteln (vereinfacht)
    String wsStatus = "nicht verbunden";
    // TODO: Hier könnte die tatsächliche WebSocket-Verbindung geprüft werden
    
    // JSON-Status erstellen
    String status = "{\"rssi\":" + String(WiFi.RSSI()) + 
                   ",\"ws_status\":\"" + wsStatus + "\"" +
                   ",\"ip\":\"" + WiFi.localIP().toString() + "\"" +
                   ",\"ssid\":\"" + WiFi.SSID() + "\"" +
                   ",\"uptime\":" + String(millis()) + 
                   ",\"connected\":" + String(WiFi.status() == WL_CONNECTED ? "true" : "false") + "}";
    
    webServer->send(200, "application/json", status);
}



void WifiManager::handleSaveLedConfig() {
    Serial.println("[Web Server] LED-Konfiguration speichern Anfrage empfangen");
    
    if (webServer->hasArg("plain")) {
        String body = webServer->arg("plain");
        Serial.printf("[Web UI] JSON-Body empfangen (Länge: %d)\n", body.length());

        // JSON validieren mit größerem Puffer
        DynamicJsonDocument doc(4096); // 4KB für große Konfigurationen
        DeserializationError error = deserializeJson(doc, body);
        
        if (error) {
            Serial.print(F("[Web UI] JSON-Parsing-Fehler: "));
            Serial.println(error.c_str());
            Serial.printf("[Web UI] Fehler-Code: %d\n", error.code());
            webServer->send(400, "text/plain", "Invalid JSON");
            return;
        }
        
        // JSON-String für Speicherung serialisieren
        String jsonString;
        serializeJson(doc, jsonString);
        
        // Konfiguration im NVS speichern
        Serial.println("[Web UI] Speichere neue LED-Konfiguration im NVS...");
        
        Preferences prefs;
        prefs.begin("led-config", false); // 'false' für Schreibzugriff
        bool saveSuccess = prefs.putString("config", jsonString);
        prefs.end();
        
        if (saveSuccess) {
            Serial.println("[Web UI] Neue LED-Konfiguration im NVS gespeichert.");
            
            // LedManager-Konfiguration neu laden
            extern LedManager ledManager;
            ledManager.loadConfig();
            Serial.println("[Web UI] LED-Konfiguration live neu geladen.");
            
            webServer->send(200, "text/plain", "Gespeichert");
        } else {
            Serial.println("[Web UI] Fehler beim Speichern in NVS.");
            webServer->send(500, "text/plain", "Speicherfehler");
        }
    } else {
        Serial.println("[Web UI] Kein JSON-Body im Request gefunden.");
        webServer->send(400, "text/plain", "Missing JSON body");
    }
}

void WifiManager::handleGetLedConfig() {
    Serial.println("[Web Server] LED-Konfiguration laden Anfrage empfangen");
    
    Preferences prefs;
    prefs.begin("led-config", true); // read-only
    
    String configJson = prefs.getString("config", "");
    prefs.end();
    
    if (configJson.length() > 0) {
        webServer->send(200, "application/json", configJson);
        Serial.println("[Web UI] LED-Konfiguration erfolgreich geladen und gesendet.");
    } else {
        // Fallback: Standard-Konfiguration erstellen und senden
        Serial.println("[Web UI] Keine gespeicherte Konfiguration gefunden, erstelle Standardwerte.");
        
        // Standard-Konfiguration erstellen
        DynamicJsonDocument doc(2048);
        
        const char* states[] = {"BOOTING", "ACCESS_POINT", "WIFI_CONNECTING", "SERVER_CONNECTING", 
                               "CONNECTED", "ERROR", "BUTTON_PRESSED", "LISTENING", "PLAYING", 
                               "OTA_UPDATE", "INSTALLING", "SUCCESS", "IDLE"};
        
        for (const char* state : states) {
            JsonObject stateObj = doc.createNestedObject(state);
            stateObj["active"] = true;
            
            // Standard-Farben basierend auf Zustand
            if (strcmp(state, "BOOTING") == 0) {
                stateObj["color"] = "FFFFFF"; // Weiß
            } else if (strcmp(state, "ACCESS_POINT") == 0) {
                stateObj["color"] = "00FFFF"; // Cyan
            } else if (strcmp(state, "WIFI_CONNECTING") == 0 || strcmp(state, "SERVER_CONNECTING") == 0) {
                stateObj["color"] = "00FFFF"; // Cyan
            } else if (strcmp(state, "CONNECTED") == 0) {
                stateObj["color"] = "001000"; // Dezentes Grün
            } else if (strcmp(state, "ERROR") == 0) {
                stateObj["color"] = "FF0000"; // Rot
            } else if (strcmp(state, "BUTTON_PRESSED") == 0) {
                stateObj["color"] = "FFFFFF"; // Weiß
            } else if (strcmp(state, "LISTENING") == 0) {
                stateObj["color"] = "00FFFF"; // Cyan
            } else if (strcmp(state, "PLAYING") == 0) {
                stateObj["color"] = "0000FF"; // Blau
            } else if (strcmp(state, "OTA_UPDATE") == 0) {
                stateObj["color"] = "800080"; // Violett
            } else if (strcmp(state, "INSTALLING") == 0) {
                stateObj["color"] = "FF8000"; // Orange
            } else if (strcmp(state, "SUCCESS") == 0) {
                stateObj["color"] = "00FF00"; // Grün
            } else if (strcmp(state, "IDLE") == 0) {
                stateObj["color"] = "000000"; // Aus
            }
            
            stateObj["brightness"] = 128;
            stateObj["effect"] = "STATIC";
            stateObj["speed"] = 50;
        }
        
        String response;
        serializeJson(doc, response);
        webServer->send(200, "application/json", response);
        Serial.println("[Web UI] Standard-Konfiguration erstellt und gesendet.");
    }
}



void WifiManager::handleSSE() {
    Serial.println("[Web Server] SSE-Verbindung für Live-Logs hergestellt");
    
    // SSE-Header setzen
    webServer->sendHeader("Content-Type", "text/event-stream");
    webServer->sendHeader("Cache-Control", "no-cache");
    webServer->sendHeader("Connection", "keep-alive");
    webServer->sendHeader("Access-Control-Allow-Origin", "*");
    webServer->send(200, "text/event-stream", "");
    
    // Sende initiale Nachricht
    String initialMessage = "data: SSE-Verbindung für Live-Logs hergestellt\n\n";
    webServer->client().print(initialMessage);
    webServer->client().flush();
    
    // Verbindung offen halten (wird vom Client verwaltet)
    Serial.println("[Web Server] SSE-Stream gestartet");
}

void WifiManager::handleGetLogs() {
    Serial.println("[Web Server] Log-Abfrage empfangen");
    
    String logsToSend = "";
    extern SemaphoreHandle_t logMutex;
    extern String logBuffer;
    
    if (xSemaphoreTake(logMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        if (logBuffer.length() > 0) {
            logsToSend = logBuffer;
            logBuffer = ""; // Puffer leeren
            Serial.printf("[Web Server] %d Bytes Logs gesendet\n", logsToSend.length());
        } else {
            Serial.println("[Web Server] Keine neuen Logs verfügbar");
        }
        xSemaphoreGive(logMutex);
    } else {
        Serial.println("[Web Server] Mutex-Time-out beim Log-Zugriff");
    }
    
    webServer->send(200, "text/plain", logsToSend);
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
    
    Serial.println("=== NVS LESEVORGANG START ===");
    config.ssid = preferences->getString(NVS_KEY_WIFI_SSID, "");
    config.password = preferences->getString(NVS_KEY_WIFI_PASS, "");
    
    Serial.printf("[NVS Read] SSID aus NVS gelesen: '%s'\n", config.ssid.c_str());
    Serial.printf("[NVS Read] Passwort aus NVS gelesen: '%s***%s' (Länge: %d)\n", 
                  config.password.substring(0, 1).c_str(), 
                  config.password.substring(config.password.length() - 1).c_str(),
                  config.password.length());
    config.staticIP = preferences->getString(NVS_KEY_STATIC_IP, "");
    config.staticGateway = preferences->getString(NVS_KEY_STATIC_GW, "");
    config.staticSubnet = preferences->getString(NVS_KEY_STATIC_SM, "");
    config.useStaticIP = preferences->getBool("use_static_ip", false);
    
    Serial.printf("WifiManager: Konfiguration geladen - SSID: %s\n", config.ssid.c_str());
    return config.ssid.length() > 0;
}

bool WifiManager::saveConfigToNVS() {
    if (!preferences) return false;
    
    Serial.println("=== NVS SPEICHERVORGANG START ===");
    Serial.printf("[NVS Write] Speichere SSID: '%s'\n", config.ssid.c_str());
    Serial.printf("[NVS Write] Speichere Passwort: '%s***%s' (Länge: %d)\n", 
                  config.password.substring(0, 1).c_str(), 
                  config.password.substring(config.password.length() - 1).c_str(),
                  config.password.length());
    
    preferences->putString(NVS_KEY_WIFI_SSID, config.ssid);
    preferences->putString(NVS_KEY_WIFI_PASS, config.password);
    preferences->putString(NVS_KEY_STATIC_IP, config.staticIP);
    preferences->putString(NVS_KEY_STATIC_GW, config.staticGateway);
    preferences->putString(NVS_KEY_STATIC_SM, config.staticSubnet);
    preferences->putBool("use_static_ip", config.useStaticIP);
    
    Serial.println("[NVS Write] Speichern abgeschlossen.");
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

// =============================================================================
// SYSTEM-KONFIGURATION
// =============================================================================

void WifiManager::handleGetSystemConfig() {
    Serial.println("[Web Server] System-Konfiguration laden Anfrage empfangen");
    
    Preferences prefs;
    prefs.begin("led-config", true); // read-only
    
    String configJson = prefs.getString("config", "");
    prefs.end();
    
    // Mikrofon-Modus aus NVS lesen
    Preferences micPrefs;
    micPrefs.begin("system-config", true); // read-only
    String micMode = micPrefs.getString("mic_mode", "always_on"); // Standard: Dauerbetrieb
    micPrefs.end();
    
    DynamicJsonDocument doc(2048);
    
    // Mikrofon-Modus hinzufügen
    doc["mic_mode"] = micMode;
    
    if (configJson.length() > 0) {
        // LED-Konfiguration gefunden, parsen und hinzufügen
        DynamicJsonDocument ledDoc(2048);
        if (deserializeJson(ledDoc, configJson) == DeserializationError::Ok) {
            doc["led_config"] = ledDoc;
        }
    } else {
        // Keine LED-Konfiguration gefunden, Standardwerte generieren
        JsonObject ledConfig = doc.createNestedObject("led_config");
        
        // Alle Zustände mit Standardwerten füllen
        const char* states[] = {"BOOTING", "NO_WIFI", "NO_WEBSOCKET", "LISTENING", "PLAYING", "IDLE", "THINKING", "OTA_UPDATE", "ERROR"};
        for (const char* state : states) {
            JsonObject stateObj = ledConfig.createNestedObject(state);
            stateObj["active"] = true;
            stateObj["color"] = "FFFFFF";
            stateObj["brightness"] = 128;
            stateObj["effect"] = "STATIC";
            stateObj["speed"] = 50;
            stateObj["minBrightness"] = 0;
            stateObj["maxBrightness"] = 255;
            stateObj["rainbowMode"] = false;
        }
    }
    
    String response;
    serializeJson(doc, response);
    webServer->send(200, "application/json", response);
    Serial.println("[Web Server] System-Konfiguration erfolgreich gesendet");
}

void WifiManager::handleSetMicMode() {
    Serial.println("[Web Server] Mikrofon-Modus setzen Anfrage empfangen");
    
    if (webServer->hasArg("plain")) {
        String jsonBody = webServer->arg("plain");
        Serial.printf("[Web Server] JSON-Body empfangen: %s\n", jsonBody.c_str());
        
        DynamicJsonDocument doc(512);
        DeserializationError error = deserializeJson(doc, jsonBody);
        
        if (error) {
            Serial.printf("[Web Server] JSON-Parsing-Fehler: %s\n", error.c_str());
            webServer->send(400, "text/plain", "Invalid JSON");
            return;
        }
        
        if (doc.containsKey("mode")) {
            String newMode = doc["mode"].as<String>();
            Serial.printf("[Web Server] Neuer Mikrofon-Modus: %s\n", newMode.c_str());
            
            // Mikrofon-Modus im NVS speichern
            Preferences prefs;
            prefs.begin("system-config", false); // Schreibzugriff
            prefs.putString("mic_mode", newMode);
            prefs.end();
            
            Serial.println("[Web Server] Mikrofon-Modus erfolgreich im NVS gespeichert");
            
            // Audio-Richtlinie aktualisieren
            extern void updateAudioState();
            updateAudioState();
            
            webServer->send(200, "text/plain", "OK");
        } else {
            Serial.println("[Web Server] 'mode' Parameter fehlt im JSON");
            webServer->send(400, "text/plain", "Missing 'mode' parameter");
        }
    } else {
        Serial.println("[Web Server] Kein JSON-Body im Request gefunden");
        webServer->send(400, "text/plain", "Missing JSON body");
    }
}

void WifiManager::handleSetMicDebug() {
    Serial.println("[Web Server] Mikrofon-Debug-Modus setzen Anfrage empfangen");
    
    if (webServer->hasArg("active")) {
        String activeValue = webServer->arg("active");
        bool newDebugMode = (activeValue == "true");
        
        // Globale Variable in main.cpp setzen
        extern volatile bool micDebugModeActive;
        micDebugModeActive = newDebugMode;
        
        Serial.printf("[Web Server] Mikrofon-Debug-Modus: %s\n", newDebugMode ? "AKTIVIERT" : "DEAKTIVIERT");
        
        // Log-Nachricht senden
        extern void logToWebClients(const char* message);
        logToWebClients(newDebugMode ? "Mikrofon-Debug-Modus: AKTIVIERT" : "Mikrofon-Debug-Modus: DEAKTIVIERT");
        
        // Audio-Richtlinie aktualisieren
        extern void updateAudioState();
        updateAudioState();
        
        webServer->send(200, "text/plain", "OK");
    } else {
        Serial.println("[Web Server] 'active' Parameter fehlt");
        webServer->send(400, "text/plain", "Missing 'active' parameter");
    }
}
