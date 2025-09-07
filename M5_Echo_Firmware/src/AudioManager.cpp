#include "AudioManager.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>

// =============================================================================
// KONSTRUKTOR & DESTRUKTOR
// =============================================================================

AudioManager::AudioManager() {
    micI2SPort = I2S_MIC_PORT;
    speakerI2SPort = I2S_SPEAKER_PORT;
    
    currentState = AudioState::IDLE;
    micEnabled = false;
    speakerEnabled = false;
    
    // Audio-Puffer initialisieren
    micBufferData = nullptr;
    speakerBufferData = nullptr;
    micBuffer.buffer = nullptr;
    speakerBuffer.buffer = nullptr;
    
    // Audio-Verarbeitung
    lastAudioProcess = 0;
    silenceCounter = 0;
    isSilenceDetected = false;
    
    // FreeRTOS-Tasks
    recordingTaskHandle = nullptr;
    playingTaskHandle = nullptr;
    recordingQueue = nullptr;
    playingQueue = nullptr;
    audioMutex = nullptr;
    
    // Event-Manager Referenz (temporär deaktiviert)
    // eventManager = nullptr;
}

AudioManager::~AudioManager() {
    // Tasks stoppen
    if (recordingTaskHandle) {
        vTaskDelete(recordingTaskHandle);
        recordingTaskHandle = nullptr;
    }
    if (playingTaskHandle) {
        vTaskDelete(playingTaskHandle);
        playingTaskHandle = nullptr;
    }
    
    // Queues löschen
    if (recordingQueue) {
        vQueueDelete(recordingQueue);
        recordingQueue = nullptr;
    }
    if (playingQueue) {
        vQueueDelete(playingQueue);
        playingQueue = nullptr;
    }
    
    // Mutex löschen
    if (audioMutex) {
        vSemaphoreDelete(audioMutex);
        audioMutex = nullptr;
    }
    
    // Puffer freigeben
    if (micBufferData) {
        free(micBufferData);
        micBufferData = nullptr;
    }
    if (speakerBufferData) {
        free(speakerBufferData);
        speakerBufferData = nullptr;
    }
    
    // I2S-Ports schließen
    i2s_driver_uninstall(micI2SPort);
    i2s_driver_uninstall(speakerI2SPort);
}

// =============================================================================
// INITIALISIERUNG
// =============================================================================

bool AudioManager::begin() {
    Serial.println("AudioManager: Initialisiere...");
    
    // Mutex erstellen
    audioMutex = xSemaphoreCreateMutex();
    if (!audioMutex) {
        Serial.println("AudioManager: Fehler beim Erstellen des Mutex");
        return false;
    }
    
    // Audio-Puffer allozieren
    micBufferData = (uint8_t*)malloc(AUDIO_RING_BUFFER_SIZE);
    speakerBufferData = (uint8_t*)malloc(AUDIO_RING_BUFFER_SIZE);
    
    if (!micBufferData || !speakerBufferData) {
        Serial.println("AudioManager: Fehler beim Allozieren der Audio-Puffer");
        return false;
    }
    
    // Ring-Puffer initialisieren
    resetRingBuffer(micBuffer);
    resetRingBuffer(speakerBuffer);
    micBuffer.buffer = micBufferData;
    speakerBuffer.buffer = speakerBufferData;
    micBuffer.size = AUDIO_RING_BUFFER_SIZE;
    speakerBuffer.size = AUDIO_RING_BUFFER_SIZE;
    
    // I2S-Mikrofon initialisieren
    if (!initI2SMicrophone()) {
        Serial.println("AudioManager: Fehler beim Initialisieren des I2S-Mikrofons");
        return false;
    }
    
    // I2S-Lautsprecher NICHT initialisieren - nur bei Bedarf
    Serial.println("AudioManager: I2S-Lautsprecher wird nur bei Bedarf initialisiert");
    
    // Verstärker-Pin initialisieren und ausschalten
    pinMode(SPEAKER_ENABLE_PIN, OUTPUT);
    digitalWrite(SPEAKER_ENABLE_PIN, LOW);
    Serial.println("[AudioManager] Initialized. Speaker is OFF.");
    
    // Queues erstellen
    recordingQueue = xQueueCreate(10, sizeof(AudioChunk));
    playingQueue = xQueueCreate(10, sizeof(AudioChunk));
    
    if (!recordingQueue || !playingQueue) {
        Serial.println("AudioManager: Fehler beim Erstellen der Queues");
        return false;
    }
    
    currentState = AudioState::IDLE;
    Serial.println("AudioManager: Initialisierung abgeschlossen");
    return true;
}

void AudioManager::update() {
    // Audio-Verarbeitung in der Hauptschleife
    unsigned long currentTime = millis();
    
    if (currentTime - lastAudioProcess > 10) { // 100Hz Update-Rate
        lastAudioProcess = currentTime;
        
        // Mikrofon verarbeiten
        if (micEnabled) {
            processMicrophone();
        }
        
        // Lautsprecher verarbeiten
        if (speakerEnabled) {
            processSpeaker();
        }
    }
}

// =============================================================================
// MIKROFON-STEUERUNG
// =============================================================================

bool AudioManager::startRecording() {
    if (xSemaphoreTake(audioMutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        Serial.println("AudioManager: Konnte Mutex nicht erlangen");
        return false;
    }
    
    if (currentState == AudioState::RECORDING) {
        xSemaphoreGive(audioMutex);
        return true; // Bereits aktiv
    }
    
    // Ring-Puffer zurücksetzen
    resetRingBuffer(micBuffer);
    
    // Recording-Task starten
    BaseType_t result = xTaskCreatePinnedToCore(
        recordingTask,
        "RecordingTask",
        AUDIO_TASK_STACK_SIZE,  // Verwende die konfigurierte Stack-Größe
        this,
        AUDIO_TASK_PRIORITY,
        &recordingTaskHandle,
        1  // Core 1 für Audio-Verarbeitung
    );
    
    if (result != pdPASS) {
        Serial.println("AudioManager: Fehler beim Erstellen der Recording-Task");
        xSemaphoreGive(audioMutex);
        return false;
    }
    
    micEnabled = true;
    currentState = AudioState::RECORDING;
    silenceCounter = 0;
    isSilenceDetected = false;
    
    xSemaphoreGive(audioMutex);
    Serial.println("AudioManager: Aufnahme gestartet");
    return true;
}

bool AudioManager::stopRecording() {
    if (xSemaphoreTake(audioMutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return false;
    }
    
    if (currentState != AudioState::RECORDING) {
        xSemaphoreGive(audioMutex);
        return true; // Bereits gestoppt
    }
    
    micEnabled = false;
    currentState = AudioState::IDLE;
    
    // Task stoppen
    if (recordingTaskHandle) {
        vTaskDelete(recordingTaskHandle);
        recordingTaskHandle = nullptr;
    }
    
    xSemaphoreGive(audioMutex);
    Serial.println("AudioManager: Aufnahme gestoppt");
    return true;
}

bool AudioManager::isRecording() const {
    return currentState == AudioState::RECORDING;
}

size_t AudioManager::getAvailableAudio() {
    if (xSemaphoreTake(audioMutex, pdMS_TO_TICKS(10)) != pdTRUE) {
        return 0;
    }
    
    size_t available = micBuffer.available;
    xSemaphoreGive(audioMutex);
    return available;
}

size_t AudioManager::readAudio(uint8_t* buffer, size_t maxLength) {
    if (!buffer || maxLength == 0) {
        return 0;
    }
    
    if (xSemaphoreTake(audioMutex, pdMS_TO_TICKS(10)) != pdTRUE) {
        return 0;
    }
    
    size_t bytesRead = readFromRingBuffer(micBuffer, buffer, maxLength);
    xSemaphoreGive(audioMutex);
    
    return bytesRead;
}

// =============================================================================
// LAUTSPRECHER-STEUERUNG
// =============================================================================

bool AudioManager::startPlaying() {
    if (xSemaphoreTake(audioMutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return false;
    }
    
    if (currentState == AudioState::PLAYING) {
        xSemaphoreGive(audioMutex);
        return true; // Bereits aktiv
    }
    
    // Ring-Puffer zurücksetzen
    resetRingBuffer(speakerBuffer);
    
    speakerEnabled = true;
    currentState = AudioState::PLAYING;
    
    // Playing-Task starten
    BaseType_t result = xTaskCreate(
        playingTask,
        "AudioPlayingTask",
        AUDIO_TASK_STACK_SIZE,
        this,
        AUDIO_TASK_PRIORITY,
        &playingTaskHandle
    );
    
    if (result != pdPASS) {
        Serial.println("AudioManager: Fehler beim Erstellen der Playing-Task");
        speakerEnabled = false;
        currentState = AudioState::IDLE;
        xSemaphoreGive(audioMutex);
        return false;
    }
    
    xSemaphoreGive(audioMutex);
    Serial.println("AudioManager: Wiedergabe gestartet");
    return true;
}

bool AudioManager::stopPlaying() {
    if (xSemaphoreTake(audioMutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return false;
    }
    
    if (currentState != AudioState::PLAYING) {
        xSemaphoreGive(audioMutex);
        return true; // Bereits gestoppt
    }
    
    // Lautsprecher sauber deaktivieren
    stopSpeaker();
    
    xSemaphoreGive(audioMutex);
    Serial.println("AudioManager: Wiedergabe gestoppt");
    return true;
}

bool AudioManager::isPlaying() const {
    return currentState == AudioState::PLAYING;
}

bool AudioManager::writeAudio(const uint8_t* data, size_t length) {
    if (!data || length == 0) {
        return false;
    }
    
    // Lautsprecher starten falls noch nicht aktiv
    startSpeaker();
    
    // Audiodaten in Ring-Puffer schreiben
    if (xSemaphoreTake(audioMutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        Serial.println("AudioManager: Fehler beim Zugriff auf Audio-Mutex");
        return false;
    }
    
    updateRingBuffer(speakerBuffer, data, length);
    xSemaphoreGive(audioMutex);
    
    return true;
}

bool AudioManager::writeAudioChunk(const AudioChunk& chunk) {
    if (!chunk.data || chunk.length == 0) {
        return false;
    }
    
    return writeAudio(chunk.data, chunk.length);
}

// =============================================================================
// PUFFER-MANAGEMENT
// =============================================================================

void AudioManager::clearMicBuffer() {
    if (xSemaphoreTake(audioMutex, pdMS_TO_TICKS(10)) != pdTRUE) {
        return;
    }
    
    resetRingBuffer(micBuffer);
    xSemaphoreGive(audioMutex);
}

void AudioManager::clearSpeakerBuffer() {
    if (xSemaphoreTake(audioMutex, pdMS_TO_TICKS(10)) != pdTRUE) {
        return;
    }
    
    resetRingBuffer(speakerBuffer);
    xSemaphoreGive(audioMutex);
}

size_t AudioManager::getMicBufferAvailable() const {
    return micBuffer.available;
}

size_t AudioManager::getSpeakerBufferAvailable() const {
    return speakerBuffer.available;
}

// =============================================================================
// AUDIO-KONFIGURATION
// =============================================================================

void AudioManager::setSampleRate(uint32_t sampleRate) {
    // I2S-Konfiguration neu laden würde hier implementiert werden
    // Für jetzt verwenden wir die Werte aus config.h
}

void AudioManager::setBitsPerSample(uint8_t bitsPerSample) {
    // I2S-Konfiguration neu laden würde hier implementiert werden
}

void AudioManager::setChannels(uint8_t channels) {
    // I2S-Konfiguration neu laden würde hier implementiert werden
}

uint32_t AudioManager::getSampleRate() const {
    return I2S_SAMPLE_RATE;
}

uint8_t AudioManager::getBitsPerSample() const {
    return I2S_BITS_PER_SAMPLE;
}

uint8_t AudioManager::getChannels() const {
    return I2S_CHANNELS;
}

// =============================================================================
// ZUSTANDSABFRAGE
// =============================================================================

AudioState AudioManager::getState() const {
    return currentState;
}

bool AudioManager::isSilence() const {
    return isSilenceDetected;
}

bool AudioManager::isBufferFull() const {
    return micBuffer.isFull;
}

bool AudioManager::isBufferEmpty() const {
    return micBuffer.isEmpty;
}

// =============================================================================
// UTILITY-METHODEN
// =============================================================================

void AudioManager::reset() {
    stopRecording();
    stopPlaying();
    clearMicBuffer();
    clearSpeakerBuffer();
    currentState = AudioState::IDLE;
    silenceCounter = 0;
    isSilenceDetected = false;
}

void AudioManager::setSilenceThreshold(int threshold) {
    silenceThreshold = threshold;
}

int AudioManager::getSilenceThreshold() const {
    return AUDIO_SILENCE_THRESHOLD;
}

unsigned long AudioManager::getLastAudioTimestamp() const {
    return lastAudioProcess;
}

// =============================================================================
// DEBUG-METHODEN
// =============================================================================

void AudioManager::printBufferStatus() {
    Serial.printf("AudioManager: Mic Buffer - Available: %d, Full: %s, Empty: %s\n",
                  micBuffer.available,
                  micBuffer.isFull ? "true" : "false",
                  micBuffer.isEmpty ? "true" : "false");
    
    Serial.printf("AudioManager: Speaker Buffer - Available: %d, Full: %s, Empty: %s\n",
                  speakerBuffer.available,
                  speakerBuffer.isFull ? "true" : "false",
                  speakerBuffer.isEmpty ? "true" : "false");
}

void AudioManager::printAudioStats() {
    Serial.printf("AudioManager: State: %d, Mic: %s, Speaker: %s, Silence: %s\n",
                  (int)currentState,
                  micEnabled ? "enabled" : "disabled",
                  speakerEnabled ? "enabled" : "disabled",
                  isSilenceDetected ? "detected" : "none");
}

// =============================================================================
// PRIVATE METHODEN
// =============================================================================

bool AudioManager::initI2SMicrophone() {
    // I2S-Konfiguration für Mikrofon
    i2s_config_t i2sConfig = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = I2S_SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE == 16 ? I2S_BITS_PER_SAMPLE_16BIT : I2S_BITS_PER_SAMPLE_32BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = I2S_BUFFER_SIZE,
        .use_apll = false,
        .tx_desc_auto_clear = false,
        .fixed_mclk = 0
    };
    
    // I2S-Pin-Konfiguration für Mikrofon
    i2s_pin_config_t pinConfig = {
        .bck_io_num = I2S_MIC_BCK_PIN,
        .ws_io_num = I2S_MIC_WS_PIN,
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num = I2S_MIC_DATA_PIN
    };
    
    // I2S-Treiber installieren
    esp_err_t err = i2s_driver_install(micI2SPort, &i2sConfig, 0, NULL);
    if (err != ESP_OK) {
        Serial.printf("AudioManager: I2S-Treiber Installation fehlgeschlagen: %d\n", err);
        return false;
    }
    
    // I2S-Pins setzen
    err = i2s_set_pin(micI2SPort, &pinConfig);
    if (err != ESP_OK) {
        Serial.printf("AudioManager: I2S-Pin-Konfiguration fehlgeschlagen: %d\n", err);
        return false;
    }
    
    Serial.println("AudioManager: I2S-Mikrofon initialisiert");
    return true;
}

bool AudioManager::initI2SSpeaker() {
    // I2S-Konfiguration für Lautsprecher
    i2s_config_t i2sConfig = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = I2S_SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE == 16 ? I2S_BITS_PER_SAMPLE_16BIT : I2S_BITS_PER_SAMPLE_32BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = I2S_BUFFER_SIZE,
        .use_apll = false,
        .tx_desc_auto_clear = true,
        .fixed_mclk = 0
    };
    
    // I2S-Pin-Konfiguration für Lautsprecher
    i2s_pin_config_t pinConfig = {
        .bck_io_num = I2S_SPEAKER_BCK_PIN,
        .ws_io_num = I2S_SPEAKER_WS_PIN,
        .data_out_num = I2S_SPEAKER_DATA_PIN,
        .data_in_num = I2S_PIN_NO_CHANGE
    };
    
    // I2S-Treiber installieren
    esp_err_t err = i2s_driver_install(speakerI2SPort, &i2sConfig, 0, NULL);
    if (err != ESP_OK) {
        Serial.printf("AudioManager: I2S-Treiber Installation fehlgeschlagen: %d\n", err);
        return false;
    }
    
    // I2S-Pins setzen
    err = i2s_set_pin(speakerI2SPort, &pinConfig);
    if (err != ESP_OK) {
        Serial.printf("AudioManager: I2S-Pin-Konfiguration fehlgeschlagen: %d\n", err);
        return false;
    }
    
    Serial.println("AudioManager: I2S-Lautsprecher initialisiert");
    return true;
}

void AudioManager::processMicrophone() {
    // Diese Methode wird von der Recording-Task aufgerufen
    // Hier könnte zusätzliche Audio-Verarbeitung stattfinden
}

void AudioManager::processSpeaker() {
    // Diese Methode wird von der Playing-Task aufgerufen
    // Hier könnte zusätzliche Audio-Verarbeitung stattfinden
}

bool AudioManager::detectSilence(const uint8_t* data, size_t length) {
    if (!data || length == 0) {
        return true;
    }
    
    // Einfache Stille-Erkennung basierend auf RMS
    int32_t sum = 0;
    for (size_t i = 0; i < length; i += 2) {
        if (i + 1 < length) {
            int16_t sample = (data[i + 1] << 8) | data[i];
            sum += abs(sample);
        }
    }
    
    int32_t average = sum / (length / 2);
    return average < AUDIO_SILENCE_THRESHOLD;
}

void AudioManager::updateRingBuffer(AudioRingBuffer& buffer, const uint8_t* data, size_t length) {
    if (!data || length == 0 || buffer.isFull) {
        return;
    }
    
    size_t bytesToWrite = min(length, buffer.size - buffer.available);
    
    for (size_t i = 0; i < bytesToWrite; i++) {
        buffer.buffer[buffer.writeIndex] = data[i];
        buffer.writeIndex = (buffer.writeIndex + 1) % buffer.size;
        buffer.available++;
    }
    
    buffer.isFull = (buffer.available >= buffer.size);
    buffer.isEmpty = (buffer.available == 0);
}

size_t AudioManager::readFromRingBuffer(AudioRingBuffer& buffer, uint8_t* data, size_t maxLength) {
    if (!data || maxLength == 0 || buffer.isEmpty) {
        return 0;
    }
    
    size_t bytesToRead = min(maxLength, buffer.available);
    
    for (size_t i = 0; i < bytesToRead; i++) {
        data[i] = buffer.buffer[buffer.readIndex];
        buffer.readIndex = (buffer.readIndex + 1) % buffer.size;
        buffer.available--;
    }
    
    buffer.isFull = (buffer.available >= buffer.size);
    buffer.isEmpty = (buffer.available == 0);
    
    return bytesToRead;
}

void AudioManager::resetRingBuffer(AudioRingBuffer& buffer) {
    buffer.readIndex = 0;
    buffer.writeIndex = 0;
    buffer.available = 0;
    buffer.isFull = false;
    buffer.isEmpty = true;
}

// =============================================================================
// FREERTOS-TASKS
// =============================================================================

void AudioManager::recordingTask(void* parameter) {
    AudioManager* manager = static_cast<AudioManager*>(parameter);
    
    uint8_t* audioBuffer = (uint8_t*)malloc(I2S_BUFFER_SIZE);
    if (!audioBuffer) {
        Serial.println("AudioManager: Fehler beim Allozieren des Audio-Buffers");
        vTaskDelete(nullptr);
        return;
    }
    
    Serial.println("AudioManager: Recording-Task gestartet");
    
    while (manager->micEnabled) {
        size_t bytesRead = 0;
        esp_err_t err = i2s_read(manager->micI2SPort, audioBuffer, I2S_BUFFER_SIZE, &bytesRead, portMAX_DELAY);
        
        if (err == ESP_OK && bytesRead > 0) {
            // Stille-Erkennung
            bool isSilence = manager->detectSilence(audioBuffer, bytesRead);
            
            if (isSilence) {
                manager->silenceCounter++;
                if (manager->silenceCounter > 10) { // 100ms Stille
                    manager->isSilenceDetected = true;
                }
            } else {
                manager->silenceCounter = 0;
                manager->isSilenceDetected = false;
            }
            
            // Daten in Ring-Puffer schreiben
            if (xSemaphoreTake(manager->audioMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
                manager->updateRingBuffer(manager->micBuffer, audioBuffer, bytesRead);
                xSemaphoreGive(manager->audioMutex);
            }
        }
        
        // Kurze Pause für andere Tasks
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    
    free(audioBuffer);
    Serial.println("AudioManager: Recording-Task beendet");
    vTaskDelete(nullptr);
}

void AudioManager::playingTask(void* parameter) {
AudioManager* manager = static_cast<AudioManager*>(parameter);

// Einen Puffer für Audiodaten und einen Puffer für Stille vorbereiten
uint8_t* audioBuffer = (uint8_t*)malloc(I2S_BUFFER_SIZE);
uint8_t* silenceBuffer = (uint8_t*)malloc(I2S_BUFFER_SIZE);
if (!audioBuffer || !silenceBuffer) {
    Serial.println("AudioManager: Fehler beim Allozieren der Wiedergabe-Puffer");
    manager->stopSpeaker(); // Wichtig: Aufräumen bei Fehler
    vTaskDelete(nullptr);
    return;
}
memset(silenceBuffer, 0, I2S_BUFFER_SIZE); // Stille-Puffer mit Nullen füllen
Serial.println("AudioManager: Playing-Task gestartet");

uint32_t lastDataTime = millis();
const uint32_t TIMEOUT_MS = 500;

while (true) {
size_t bytesToWrite = 0;

if (xSemaphoreTake(manager->audioMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
    bytesToWrite = manager->readFromRingBuffer(manager->speakerBuffer, audioBuffer, I2S_BUFFER_SIZE);
    xSemaphoreGive(manager->audioMutex);
}

if (bytesToWrite > 0) {
    // Echte Audiodaten erhalten, sende audioBuffer
    lastDataTime = millis();
    size_t bytesWritten = 0;
    i2s_write(manager->speakerI2SPort, audioBuffer, bytesToWrite, &bytesWritten, portMAX_DELAY);
} else {
    // Keine Daten verfügbar, prüfe auf Timeout
    if (millis() - lastDataTime > TIMEOUT_MS) {
        Serial.println("[AudioManager] Playback Timeout. Stopping speaker...");
        break; // Schleife verlassen und Task beenden
    }
    // Noch kein Timeout, sende Stille, um Rauschen zu vermeiden
    size_t bytesWritten = 0;
    i2s_write(manager->speakerI2SPort, silenceBuffer, I2S_BUFFER_SIZE, &bytesWritten, portMAX_DELAY);
}
vTaskDelay(pdMS_TO_TICKS(1));
}

free(audioBuffer);
free(silenceBuffer);

manager->stopSpeaker();

Serial.println("[AudioManager] PlayingTask DELETED.");
vTaskDelete(nullptr);


}

// =============================================================================
// LAUTSPRECHER-ZUSTANDSVERWALTUNG
// =============================================================================

void AudioManager::startSpeaker() {
    if (m_speakerState == SpeakerState::ACTIVE) {
        return; // Bereits aktiv
    }
    
    Serial.println("[AudioManager] Speaker START requested...");
    
    // Verstärker physisch einschalten
    enableAmplifier();
    
    // I2S-Treiber installieren
    if (!initI2SSpeaker()) {
        Serial.println("AudioManager: Fehler beim Initialisieren des I2S-Lautsprechers");
        disableAmplifier(); // Verstärker wieder ausschalten bei Fehler
        return;
    }
    
    Serial.println("[AudioManager] I2S driver INSTALLED.");
    
    // Playing-Task starten falls noch nicht aktiv
    if (playingTaskHandle == nullptr) {
        BaseType_t result = xTaskCreate(
            playingTask,
            "AudioPlayingTask",
            AUDIO_TASK_STACK_SIZE,  // Verwende die konfigurierte Stack-Größe
            this,
            AUDIO_TASK_PRIORITY,
            &playingTaskHandle
        );
        
        if (result != pdPASS) {
            Serial.println("AudioManager: Fehler beim Erstellen der Playing-Task");
            i2s_driver_uninstall(speakerI2SPort);
            disableAmplifier(); // Verstärker wieder ausschalten bei Fehler
            return;
        }
        
        Serial.println("[AudioManager] PlayingTask CREATED.");
    }
    
    speakerEnabled = true;
    currentState = AudioState::PLAYING;
    m_speakerState = SpeakerState::ACTIVE;
    
    Serial.println("AudioManager: Lautsprecher aktiviert");
}

void AudioManager::stopSpeaker() {
    if (m_speakerState == SpeakerState::INACTIVE) {
        return; // Bereits inaktiv
    }
    
    Serial.println("[AudioManager] Speaker STOP requested...");
    
    // I2S-Treiber deinstallieren
    i2s_driver_uninstall(speakerI2SPort);
    Serial.println("[AudioManager] I2S driver UNINSTALLED.");
    
    // Verstärker physisch ausschalten
    disableAmplifier();
    
    // Task-Handle zurücksetzen
    playingTaskHandle = nullptr;
    
    speakerEnabled = false;
    currentState = AudioState::IDLE;
    m_speakerState = SpeakerState::INACTIVE;
    
    Serial.println("AudioManager: Lautsprecher deaktiviert");
}

// =============================================================================
// VERSTÄRKER-STEUERUNG
// =============================================================================

void AudioManager::enableAmplifier() {
    Serial.println("[AudioManager] Powering ON amplifier chip...");
    digitalWrite(SPEAKER_ENABLE_PIN, HIGH);
    delay(10); // Kurze Verzögerung von 10ms zur Stabilisierung
    Serial.println("[AudioManager] Amplifier chip powered ON.");
}

void AudioManager::disableAmplifier() {
    Serial.println("[AudioManager] Powering OFF amplifier chip...");
    digitalWrite(SPEAKER_ENABLE_PIN, LOW);
    Serial.println("[AudioManager] Amplifier chip powered OFF.");
}

// =============================================================================
// EVENT-INTEGRATION
// =============================================================================

void AudioManager::setEventManager(EventManager* manager) {
    // eventManager = manager;  // Temporär deaktiviert
    Serial.println("AudioManager: EventManager gesetzt (deaktiviert)");
}

bool AudioManager::playChunk(const uint8_t* data, size_t size) {
    if (!data || size == 0) {
        return false;
    }
    
    // Lautsprecher starten falls noch nicht aktiv
    startSpeaker();
    
    // Audiodaten in Ring-Puffer schreiben
    if (xSemaphoreTake(audioMutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        Serial.println("AudioManager: Fehler beim Zugriff auf Audio-Mutex");
        return false;
    }
    
    updateRingBuffer(speakerBuffer, data, size);
    xSemaphoreGive(audioMutex);
    
    return true;
}

bool AudioManager::playTestTone() {
    Serial.println("AudioManager: Spiele Test-Ton ab...");
    
    // Lautsprecher starten falls noch nicht aktiv
    startSpeaker();
    
    // Test-Ton generieren (einfache Stille)
    const size_t testToneSize = 1024;
    uint8_t* testTone = (uint8_t*)malloc(testToneSize);
    if (!testTone) {
        Serial.println("AudioManager: Fehler beim Allozieren des Test-Ton-Buffers");
        return false;
    }
    
    // Einfacher Test-Ton (alle Samples auf 0 für Stille)
    memset(testTone, 0, testToneSize);
    
    // Test-Ton in Ring-Puffer schreiben
    if (xSemaphoreTake(audioMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        updateRingBuffer(speakerBuffer, testTone, testToneSize);
        xSemaphoreGive(audioMutex);
        Serial.println("AudioManager: Test-Ton in Ring-Puffer geschrieben");
    }
    
    free(testTone);
    return true;
}
