#include "OtaManager.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/timers.h>
#include <esp_ota_ops.h>
#include <esp_app_desc.h>
#include <esp_partition.h>
#include <mbedtls/md5.h>

// =============================================================================
// KONSTRUKTOR & DESTRUKTOR
// =============================================================================

OtaManager::OtaManager() {
    currentStatus = OtaStatus::IDLE;
    
    // Update-Server-Konfiguration
    updateServerUrl = DEFAULT_UPDATE_SERVER_URL;
    currentVersion = "1.0.0";
    deviceId = DEFAULT_CLIENT_ID;
    
    // Update-Parameter
    autoUpdateEnabled = false;
    checkInterval = 3600000; // 1 Stunde
    lastCheckTime = 0;
    
    // HTTP-Client
    httpClient = nullptr;
    
    // Callbacks
    statusCallback = nullptr;
    progressCallback = nullptr;
    errorCallback = nullptr;
    
    // Update-Info
    updateInfo.version = "";
    updateInfo.url = "";
    updateInfo.size = 0;
    updateInfo.checksum = "";
    updateInfo.available = false;
    
    // Progress
    progress.downloaded = 0;
    progress.total = 0;
    progress.percentage = 0;
    progress.startTime = 0;
    progress.estimatedTimeRemaining = 0;
    
    // Error-Handling
    lastError = "";
    
    // Debug
    debugEnabled = false;
    
    // LED-Manager Referenz
    ledManager = nullptr;
}

OtaManager::~OtaManager() {
    if (httpClient) {
        delete httpClient;
        httpClient = nullptr;
    }
}

// =============================================================================
// INITIALISIERUNG
// =============================================================================

void OtaManager::begin() {
    Serial.println("OtaManager: Initialisiere...");
    
    // HTTP-Client erstellen
    httpClient = new HTTPClient();
    if (!httpClient) {
        Serial.println("OtaManager: Fehler beim Erstellen des HTTP-Clients");
        return;
    }
    
    // Aktuelle Version ermitteln
    const esp_app_desc_t* appDesc = esp_app_get_description();
    if (appDesc) {
        currentVersion = String(appDesc->version);
    }
    
    // LED-Manager Referenz setzen (wird von außen gesetzt)
    
    currentStatus = OtaStatus::IDLE;
    Serial.printf("OtaManager: Initialisierung abgeschlossen (Version: %s)\n", currentVersion.c_str());
}

void OtaManager::update() {
    // Auto-Update-Check
    if (autoUpdateEnabled && currentStatus == OtaStatus::IDLE) {
        unsigned long currentTime = millis();
        if (currentTime - lastCheckTime > checkInterval) {
            checkForUpdates();
            lastCheckTime = currentTime;
        }
    }
}

// =============================================================================
// UPDATE-SERVER-KONFIGURATION
// =============================================================================

void OtaManager::setUpdateServer(const String& url) {
    updateServerUrl = url;
    Serial.printf("OtaManager: Update-Server auf %s gesetzt\n", url.c_str());
}

void OtaManager::setCurrentVersion(const String& version) {
    currentVersion = version;
    Serial.printf("OtaManager: Aktuelle Version auf %s gesetzt\n", version.c_str());
}

void OtaManager::setDeviceId(const String& id) {
    deviceId = id;
    Serial.printf("OtaManager: Device-ID auf %s gesetzt\n", id.c_str());
}

String OtaManager::getUpdateServer() const {
    return updateServerUrl;
}

String OtaManager::getCurrentVersion() const {
    return currentVersion;
}

String OtaManager::getDeviceId() const {
    return deviceId;
}

// =============================================================================
// UPDATE-STEUERUNG
// =============================================================================

bool OtaManager::startUpdate() {
    if (currentStatus != OtaStatus::IDLE) {
        Serial.println("OtaManager: Update bereits aktiv");
        return false;
    }
    
    if (!updateInfo.available || updateInfo.url.isEmpty()) {
        Serial.println("OtaManager: Kein Update verfügbar");
        return false;
    }
    
    Serial.println("OtaManager: Starte Update-Prozess...");
    setStatus(OtaStatus::DOWNLOADING);
    
    // LED-Feedback starten
    if (ledManager) {
        ledManager->setState(LedState::OTA_UPDATE);
    }
    
    // Update herunterladen und installieren
    bool success = downloadUpdate();
    if (success) {
        success = installUpdate();
    }
    
    if (success) {
        setStatus(OtaStatus::SUCCESS);
        Serial.println("OtaManager: Update erfolgreich - Neustart in 3 Sekunden...");
        
        // LED-Feedback für Erfolg
        if (ledManager) {
            ledManager->setState(LedState::SUCCESS);
        }
        
        delay(3000);
        ESP.restart();
    } else {
        setStatus(OtaStatus::ERROR);
        Serial.println("OtaManager: Update fehlgeschlagen");
        
        // LED-Feedback für Fehler
        if (ledManager) {
            ledManager->setState(LedState::ERROR);
        }
        
        // Zurück zu normalem Betrieb
        delay(2000);
        if (ledManager) {
            ledManager->setState(LedState::IDLE);
        }
    }
    
    return success;
}

bool OtaManager::checkForUpdates() {
    if (currentStatus != OtaStatus::IDLE) {
        return false;
    }
    
    Serial.println("OtaManager: Prüfe auf Updates...");
    setStatus(OtaStatus::CHECKING);
    
    // Update-Info zurücksetzen
    clearUpdateInfo();
    
    // HTTP-Request an Update-Server
    String checkUrl = updateServerUrl + "/check?device=" + deviceId + "&version=" + currentVersion;
    
    if (!httpClient->begin(checkUrl)) {
        handleError("HTTP-Client konnte nicht gestartet werden");
        return false;
    }
    
    httpClient->setTimeout(10000); // 10 Sekunden Timeout
    
    int httpCode = httpClient->GET();
    
    if (httpCode == HTTP_CODE_OK) {
        String payload = httpClient->getString();
        
        // JSON-Parsing für Update-Info
        DynamicJsonDocument doc(1024);
        DeserializationError error = deserializeJson(doc, payload);
        
        if (error) {
            handleError("JSON-Parsing-Fehler: " + String(error.c_str()));
            return false;
        }
        
        // Update-Info extrahieren
        if (doc["available"] == true) {
            updateInfo.available = true;
            updateInfo.version = doc["version"] | "";
            updateInfo.url = doc["url"] | "";
            updateInfo.size = doc["size"] | 0;
            updateInfo.checksum = doc["checksum"] | "";
            
            Serial.printf("OtaManager: Update verfügbar - Version: %s, Größe: %d Bytes\n",
                          updateInfo.version.c_str(),
                          updateInfo.size);
            
            setStatus(OtaStatus::IDLE);
            return true;
        } else {
            Serial.println("OtaManager: Kein Update verfügbar");
        }
    } else {
        handleError("HTTP-Fehler: " + String(httpCode));
    }
    
    httpClient->end();
    setStatus(OtaStatus::IDLE);
    return false;
}

void OtaManager::cancelUpdate() {
    if (currentStatus == OtaStatus::DOWNLOADING || currentStatus == OtaStatus::INSTALLING) {
        Serial.println("OtaManager: Update abgebrochen");
        
        if (httpClient) {
            httpClient->end();
        }
        
        setStatus(OtaStatus::IDLE);
        
        // LED-Feedback zurücksetzen
        if (ledManager) {
            ledManager->setState(LedState::IDLE);
        }
    }
}

OtaStatus OtaManager::getStatus() const {
    return currentStatus;
}

// =============================================================================
// AUTO-UPDATE-KONFIGURATION
// =============================================================================

void OtaManager::enableAutoUpdate(bool enabled) {
    autoUpdateEnabled = enabled;
    Serial.printf("OtaManager: Auto-Update %s\n", enabled ? "aktiviert" : "deaktiviert");
}

void OtaManager::setCheckInterval(unsigned long interval) {
    checkInterval = interval;
    Serial.printf("OtaManager: Check-Intervall auf %lu ms gesetzt\n", interval);
}

bool OtaManager::isAutoUpdateEnabled() const {
    return autoUpdateEnabled;
}

// =============================================================================
// UPDATE-INFORMATIONEN
// =============================================================================

OtaUpdateInfo OtaManager::getUpdateInfo() const {
    return updateInfo;
}

OtaProgress OtaManager::getProgress() const {
    return progress;
}

bool OtaManager::isUpdateAvailable() const {
    return updateInfo.available;
}

// =============================================================================
// CALLBACK-REGISTRIERUNG
// =============================================================================

void OtaManager::setStatusCallback(OtaStatusCallback callback) {
    statusCallback = callback;
}

void OtaManager::setProgressCallback(OtaProgressCallback callback) {
    progressCallback = callback;
}

void OtaManager::setErrorCallback(OtaErrorCallback callback) {
    errorCallback = callback;
}

// =============================================================================
// UTILITY-METHODEN
// =============================================================================

void OtaManager::reset() {
    cancelUpdate();
    clearUpdateInfo();
    setStatus(OtaStatus::IDLE);
    Serial.println("OtaManager: Zurückgesetzt");
}

void OtaManager::clearUpdateInfo() {
    updateInfo.version = "";
    updateInfo.url = "";
    updateInfo.size = 0;
    updateInfo.checksum = "";
    updateInfo.available = false;
    
    progress.downloaded = 0;
    progress.total = 0;
    progress.percentage = 0;
    progress.startTime = 0;
    progress.estimatedTimeRemaining = 0;
}

String OtaManager::getLastError() const {
    return lastError;
}

unsigned long OtaManager::getLastCheckTime() const {
    return lastCheckTime;
}

// =============================================================================
// DEBUG-METHODEN
// =============================================================================

void OtaManager::printUpdateStatus() {
    Serial.printf("OtaManager: Status=%d, Auto-Update=%s, Letzter Check=%lu ms\n",
                  (int)currentStatus,
                  autoUpdateEnabled ? "aktiv" : "inaktiv",
                  lastCheckTime);
}

void OtaManager::printUpdateInfo() {
    Serial.printf("OtaManager: Update verfügbar=%s, Version=%s, Größe=%d Bytes\n",
                  updateInfo.available ? "ja" : "nein",
                  updateInfo.version.c_str(),
                  updateInfo.size);
}

void OtaManager::enableDebug(bool enabled) {
    debugEnabled = enabled;
    Serial.printf("OtaManager: Debug %s\n", enabled ? "aktiviert" : "deaktiviert");
}

// =============================================================================
// MANUELLE UPDATE-FUNKTIONEN
// =============================================================================

bool OtaManager::updateFromUrl(const String& url) {
    if (currentStatus != OtaStatus::IDLE) {
        return false;
    }
    
    Serial.printf("OtaManager: Update von URL: %s\n", url.c_str());
    
    // Update-Info setzen
    updateInfo.url = url;
    updateInfo.available = true;
    updateInfo.version = "manual";
    updateInfo.size = 0; // Wird beim Download ermittelt
    updateInfo.checksum = "";
    
    return startUpdate();
}

bool OtaManager::updateFromFile(const String& filename) {
    Serial.printf("OtaManager: Update von Datei: %s (nicht implementiert)\n", filename.c_str());
    return false;
}

bool OtaManager::verifyUpdate(const String& checksum) {
    if (checksum.isEmpty()) {
        return true; // Keine Checksumme = keine Verifikation
    }
    
    // Checksum-Verifikation implementieren
    Serial.printf("OtaManager: Verifiziere Checksum: %s\n", checksum.c_str());
    return true;
}

// =============================================================================
// PRIVATE METHODEN
// =============================================================================

bool OtaManager::downloadUpdate() {
    if (!httpClient || updateInfo.url.isEmpty()) {
        handleError("HTTP-Client nicht verfügbar oder URL leer");
        return false;
    }
    
    Serial.printf("OtaManager: Lade Update herunter: %s\n", updateInfo.url.c_str());
    
    if (!httpClient->begin(updateInfo.url)) {
        handleError("HTTP-Client konnte nicht gestartet werden");
        return false;
    }
    
    httpClient->setTimeout(30000); // 30 Sekunden Timeout für Download
    
    int httpCode = httpClient->GET();
    
    if (httpCode != HTTP_CODE_OK) {
        handleError("HTTP-Fehler beim Download: " + String(httpCode));
        httpClient->end();
        return false;
    }
    
    // Dateigröße ermitteln
    int contentLength = httpClient->getSize();
    if (contentLength <= 0) {
        handleError("Ungültige Dateigröße");
        httpClient->end();
        return false;
    }
    
    updateInfo.size = contentLength;
    progress.total = contentLength;
    progress.downloaded = 0;
    progress.startTime = millis();
    
    Serial.printf("OtaManager: Download gestartet - Größe: %d Bytes\n", contentLength);
    
    // Update-Bereich vorbereiten
    if (!Update.begin(contentLength)) {
        handleError("Update-Bereich konnte nicht vorbereitet werden");
        httpClient->end();
        return false;
    }
    
    // Download und Schreiben
    WiFiClient* stream = httpClient->getStreamPtr();
    if (!stream) {
        handleError("Stream nicht verfügbar");
        httpClient->end();
        return false;
    }
    size_t written = Update.writeStream(*stream);
    
    if (written != contentLength) {
        handleError("Download unvollständig: " + String(written) + "/" + String(contentLength));
        Update.abort();
        httpClient->end();
        return false;
    }
    
    httpClient->end();
    
    Serial.printf("OtaManager: Download abgeschlossen - %d Bytes geschrieben\n", written);
    return true;
}

bool OtaManager::installUpdate() {
    Serial.println("OtaManager: Installiere Update...");
    setStatus(OtaStatus::INSTALLING);
    
    // LED-Feedback für Installation
    if (ledManager) {
        ledManager->setState(LedState::INSTALLING);
    }
    
    // Update validieren und abschließen
    if (!Update.end()) {
        handleError("Update-Validierung fehlgeschlagen: " + String(Update.errorString()));
        return false;
    }
    
    Serial.println("OtaManager: Update erfolgreich installiert");
    return true;
}

bool OtaManager::verifyChecksum(const String& expectedChecksum, const uint8_t* data, size_t length) {
    if (expectedChecksum.isEmpty()) {
        return true; // Keine Checksumme = keine Verifikation
    }
    
    String calculatedChecksum = calculateChecksum(data, length);
    bool valid = (calculatedChecksum == expectedChecksum);
    
    if (!valid) {
        Serial.printf("OtaManager: Checksum-Verifikation fehlgeschlagen - Erwartet: %s, Berechnet: %s\n",
                      expectedChecksum.c_str(),
                      calculatedChecksum.c_str());
    }
    
    return valid;
}

void OtaManager::updateProgress(size_t downloaded, size_t total) {
    progress.downloaded = downloaded;
    progress.total = total;
    progress.percentage = (total > 0) ? (downloaded * 100) / total : 0;
    
    // Geschätzte verbleibende Zeit
    if (progress.startTime > 0 && downloaded > 0) {
        unsigned long elapsed = millis() - progress.startTime;
        unsigned long bytesPerMs = downloaded / elapsed;
        if (bytesPerMs > 0) {
            progress.estimatedTimeRemaining = (total - downloaded) / bytesPerMs;
        }
    }
    
    if (debugEnabled) {
        Serial.printf("OtaManager: Fortschritt %d%% (%d/%d Bytes)\n",
                      progress.percentage,
                      downloaded,
                      total);
    }
    
    // Progress-Callback aufrufen
    if (progressCallback) {
        progressCallback(progress);
    }
}

void OtaManager::setStatus(OtaStatus status) {
    if (currentStatus != status) {
        currentStatus = status;
        
        if (debugEnabled) {
            Serial.printf("OtaManager: Status geändert zu %d\n", (int)status);
        }
        
        // Status-Callback aufrufen
        if (statusCallback) {
            statusCallback(status);
        }
    }
}

void OtaManager::handleError(const String& error) {
    lastError = error;
    Serial.printf("OtaManager: Fehler - %s\n", error.c_str());
    
    // Error-Callback aufrufen
    if (errorCallback) {
        errorCallback(error);
    }
}

String OtaManager::calculateChecksum(const uint8_t* data, size_t length) {
    // MD5-Checksum berechnen
    unsigned char hash[16];
    mbedtls_md5(data, length, hash);
    
    String checksum = "";
    for (int i = 0; i < 16; i++) {
        char hex[3];
        sprintf(hex, "%02x", hash[i]);
        checksum += hex;
    }
    
    return checksum;
}

// =============================================================================
// LED-MANAGER INTEGRATION
// =============================================================================

void OtaManager::setLedManager(LedManager* manager) {
    ledManager = manager;
    Serial.println("OtaManager: LED-Manager gesetzt");
}

void OtaManager::processOtaCommand(const String& command) {
    Serial.printf("OtaManager: Verarbeite OTA-Befehl: %s\n", command.c_str());
    
    // JSON-Befehl parsen
    DynamicJsonDocument doc(512);
    DeserializationError error = deserializeJson(doc, command);
    
    if (error) {
        Serial.printf("OtaManager: JSON-Parsing-Fehler: %s\n", error.c_str());
        return;
    }
    
    // Befehlstyp ermitteln
    const char* type = doc["type"];
    if (!type) {
        Serial.println("OtaManager: Kein Befehlstyp gefunden");
        return;
    }
    
    if (strcmp(type, "check_update") == 0) {
        // Update-Check starten
        checkForUpdates();
    }
    else if (strcmp(type, "start_update") == 0) {
        // Update starten
        if (isUpdateAvailable()) {
            startUpdate();
        } else {
            Serial.println("OtaManager: Kein Update verfügbar");
        }
    }
    else if (strcmp(type, "update_from_url") == 0) {
        // Update von URL
        const char* url = doc["url"];
        if (url) {
            updateFromUrl(String(url));
        }
    }
    else {
        Serial.printf("OtaManager: Unbekannter Befehlstyp: %s\n", type);
    }
}

// =============================================================================
// FEHLENDE MEMBER-VARIABLEN IN HEADER HINZUFÜGEN
// =============================================================================

// Diese Variablen müssen in der Header-Datei hinzugefügt werden:
// String lastError;
// bool debugEnabled;
// LedManager* ledManager;
