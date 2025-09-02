#ifndef OTA_MANAGER_H
#define OTA_MANAGER_H

#include <Arduino.h>
#include <HTTPClient.h>
#include <Update.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>
#include "config.h"
#include "LedManager.h"

// OTA-Update-Status
enum class OtaStatus {
    IDLE,           // Ruhezustand
    CHECKING,       // Prüfe auf Updates
    DOWNLOADING,    // Download läuft
    INSTALLING,     // Installation läuft
    SUCCESS,        // Update erfolgreich
    ERROR           // Fehler aufgetreten
};

// OTA-Update-Informationen
struct OtaUpdateInfo {
    String version;
    String url;
    size_t size;
    String checksum;
    bool available;
};

// OTA-Update-Fortschritt
struct OtaProgress {
    size_t downloaded;
    size_t total;
    int percentage;
    unsigned long startTime;
    unsigned long estimatedTimeRemaining;
};

class OtaManager {
private:
    OtaStatus currentStatus;
    OtaUpdateInfo updateInfo;
    OtaProgress progress;
    
    // Update-Server-Konfiguration
    String updateServerUrl;
    String currentVersion;
    String deviceId;
    
    // Update-Parameter
    bool autoUpdateEnabled;
    unsigned long checkInterval;
    unsigned long lastCheckTime;
    
    // HTTP-Client
    HTTPClient* httpClient;
    
    // Callback-Funktionen
    typedef void (*OtaStatusCallback)(OtaStatus status);
    typedef void (*OtaProgressCallback)(const OtaProgress& progress);
    typedef void (*OtaErrorCallback)(const String& error);
    
    OtaStatusCallback statusCallback;
    OtaProgressCallback progressCallback;
    OtaErrorCallback errorCallback;
    
    // Error-Handling und Debug
    String lastError;
    bool debugEnabled;
    
    // LED-Manager Integration
    LedManager* ledManager;
    
    // Private Methoden
    bool downloadUpdate();
    bool installUpdate();
    bool verifyChecksum(const String& expectedChecksum, const uint8_t* data, size_t length);
    void updateProgress(size_t downloaded, size_t total);
    void setStatus(OtaStatus status);
    void handleError(const String& error);
    String calculateChecksum(const uint8_t* data, size_t length);

public:
    // Konstruktor & Destruktor
    OtaManager();
    ~OtaManager();
    
    // Initialisierung
    void begin();
    void update();
    
    // Update-Server-Konfiguration
    void setUpdateServer(const String& url);
    void setCurrentVersion(const String& version);
    void setDeviceId(const String& id);
    String getUpdateServer() const;
    String getCurrentVersion() const;
    String getDeviceId() const;
    
    // Update-Steuerung
    bool startUpdate();
    bool checkForUpdates();
    void cancelUpdate();
    OtaStatus getStatus() const;
    
    // Auto-Update-Konfiguration
    void enableAutoUpdate(bool enabled);
    void setCheckInterval(unsigned long interval);
    bool isAutoUpdateEnabled() const;
    
    // Update-Informationen
    OtaUpdateInfo getUpdateInfo() const;
    OtaProgress getProgress() const;
    bool isUpdateAvailable() const;
    
    // Callback-Registrierung
    void setStatusCallback(OtaStatusCallback callback);
    void setProgressCallback(OtaProgressCallback callback);
    void setErrorCallback(OtaErrorCallback callback);
    
    // Utility-Methoden
    void reset();
    void clearUpdateInfo();
    String getLastError() const;
    unsigned long getLastCheckTime() const;
    
    // Debug-Methoden
    void printUpdateStatus();
    void printUpdateInfo();
    void enableDebug(bool enabled);
    
    // Manuelle Update-Funktionen
    bool updateFromUrl(const String& url);
    bool updateFromFile(const String& filename);
    bool verifyUpdate(const String& checksum);
    
    // LED-Manager Integration
    void setLedManager(LedManager* manager);
    void processOtaCommand(const String& command);
};

#endif // OTA_MANAGER_H
