#ifndef AUDIO_MANAGER_H
#define AUDIO_MANAGER_H

#include <Arduino.h>
#include <driver/i2s.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <M5Atom.h>
#include "config.h"

// Forward-Deklaration
class EventManager;

// Audio-Zustände
enum class AudioState {
    IDLE,           // Ruhezustand
    RECORDING,      // Aufnahme läuft
    PLAYING,        // Wiedergabe läuft
    BUFFERING,      // Pufferung läuft
    ERROR           // Fehler aufgetreten
};

// Audio-Ring-Puffer für Streaming
struct AudioRingBuffer {
    uint8_t* buffer;
    size_t size;
    size_t readIndex;
    size_t writeIndex;
    size_t available;
    bool isFull;
    bool isEmpty;
};

// Audio-Chunk für Streaming
struct AudioChunk {
    uint8_t* data;
    size_t length;
    unsigned long timestamp;
    bool isSilence;
};

class AudioManager {
private:
    // I2S-Konfiguration
    i2s_port_t micI2SPort;
    i2s_port_t speakerI2SPort;
    
    // Audio-Puffer
    AudioRingBuffer micBuffer;
    AudioRingBuffer speakerBuffer;
    uint8_t* micBufferData;
    uint8_t* speakerBufferData;
    
    // Neuer Puffer für getCurrentAudioLevel()
    uint8_t* lastMicBuffer;
    size_t lastMicBufferSize;
    
    // Zustandsverwaltung
    AudioState currentState;
    bool micEnabled;
    bool speakerEnabled;
    
    // Lautsprecher-Zustandsverwaltung
    enum class SpeakerState { INACTIVE, ACTIVE };
    SpeakerState m_speakerState = SpeakerState::INACTIVE;
    
    // Audio-Verarbeitung
    unsigned long lastAudioProcess;
    int silenceCounter;
    bool isSilenceDetected;
    int silenceThreshold;
    
    // FreeRTOS-Tasks und Synchronisation
    TaskHandle_t recordingTaskHandle;
    TaskHandle_t playingTaskHandle;
    QueueHandle_t recordingQueue;
    QueueHandle_t playingQueue;
    SemaphoreHandle_t audioMutex;
    
    // Private Methoden
    bool initI2SMicrophone();
    bool initI2SSpeaker();
    void processMicrophone();
    void processSpeaker();
    bool detectSilence(const uint8_t* data, size_t length);
    void updateRingBuffer(AudioRingBuffer& buffer, const uint8_t* data, size_t length);
    size_t readFromRingBuffer(AudioRingBuffer& buffer, uint8_t* data, size_t maxLength);
    void resetRingBuffer(AudioRingBuffer& buffer);
    
    // FreeRTOS-Task-Funktionen
    static void recordingTask(void* parameter);
    static void playingTask(void* parameter);
    
    // Lautsprecher-Zustandsmethoden
    void startSpeaker();
    void stopSpeaker();
    
    // Verstärker-Steuerung
    void enableAmplifier();
    void disableAmplifier();
    
    // Event-Manager Referenz
    EventManager* eventManager;

public:
    // Konstruktor & Destruktor
    AudioManager();
    ~AudioManager();
    
    // Initialisierung
    bool begin();
    void update();
    
    // Mikrofon-Steuerung
    bool startRecording();
    bool stopRecording();
    bool isRecording() const;
    size_t getAvailableAudio();
    size_t readAudio(uint8_t* buffer, size_t maxLength);
    
    // Lautsprecher-Steuerung
    bool startPlaying();
    bool stopPlaying();
    bool isPlaying() const;
    bool writeAudio(const uint8_t* data, size_t length);
    bool writeAudioChunk(const AudioChunk& chunk);
    
    // Puffer-Management
    void clearMicBuffer();
    void clearSpeakerBuffer();
    size_t getMicBufferAvailable() const;
    size_t getSpeakerBufferAvailable() const;
    
    // Audio-Konfiguration
    void setSampleRate(uint32_t sampleRate);
    void setBitsPerSample(uint8_t bitsPerSample);
    void setChannels(uint8_t channels);
    uint32_t getSampleRate() const;
    uint8_t getBitsPerSample() const;
    uint8_t getChannels() const;
    
    // Zustandsabfrage
    AudioState getState() const;
    bool isSilence() const;
    bool isBufferFull() const;
    bool isBufferEmpty() const;
    
    // Utility-Methoden
    void reset();
    void setSilenceThreshold(int threshold);
    int getSilenceThreshold() const;
    unsigned long getLastAudioTimestamp() const;
    
    // Debug-Methoden
    void printBufferStatus();
    void printAudioStats();
    
    // Event-Integration
    void setEventManager(EventManager* manager);
    bool playChunk(const uint8_t* data, size_t size);
    
    // Test-Funktionen
    bool playTestTone();
    
    // Debug-Funktionen
    uint16_t getCurrentAudioLevel();
    
    // Audio-Verstärkung (entsprechend dem Beispiel)
    void amplifyAudioData(uint8_t* data, size_t length);
    
    // M5Atom.h-basierte I2S-Initialisierung (bewährte Lösung)
    void InitI2SSpeakerOrMic(int mode);
};

#endif // AUDIO_MANAGER_H
