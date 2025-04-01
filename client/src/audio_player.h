#ifndef AUDIO_PLAYER_H
#define AUDIO_PLAYER_H

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <AudioToolbox/AudioToolbox.h>
#include <CoreAudio/CoreAudio.h>

#include "../../common/include/wav_header.h"

class AudioPlayer {
private:
    WavHeader header;
    std::vector<char> audioData;
    
    // Playback state
    std::atomic<bool> playing;
    std::atomic<bool> shouldStop;
    std::atomic<unsigned int> currentPosition;
    std::thread playbackThread;
    std::mutex mutex;
    std::condition_variable cv;
    
    // Audio Unit variables
    AudioUnit audioUnit;
    AudioStreamBasicDescription audioFormat;
    
    // Network synchronization timestamp
    std::atomic<uint64_t> syncTimestamp;
    
    // AudioUnit render callback
    static OSStatus RenderCallback(void *inRefCon,
                                  AudioUnitRenderActionFlags *ioActionFlags,
                                  const AudioTimeStamp *inTimeStamp,
                                  UInt32 inBusNumber, UInt32 inNumberFrames,
                                  AudioBufferList *ioData);
    
    bool setupAudioUnit();

public:
    AudioPlayer();
    ~AudioPlayer();
    
    // Initialize with header and audio data
    bool initialize(const WavHeader& wavHeader);
    
    // Add audio data (for streaming)
    void addAudioData(const std::vector<char>& data);
    
    // Clear audio data
    void clearAudioData();
    
    // Control functions
    bool play();
    bool stop();
    bool pause();
    bool seekToPosition(double seconds);
    
    // Status functions
    double getPositionInSeconds() const;
    double getDurationInSeconds() const;
    bool isPlaying() const;
    
    // Network synchronization functions
    bool setSyncTimestamp(uint64_t timestamp);
    uint64_t getSyncTimestamp() const;
    bool syncWithTimestamp(uint64_t timestamp, double positionInSeconds);
};

#endif // AUDIO_PLAYER_H