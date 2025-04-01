#include "audio_player.h"
#include <chrono>
#include <cmath>
#include <iostream>

AudioPlayer::AudioPlayer()
    : playing(false), shouldStop(false), currentPosition(0), syncTimestamp(0) {}

AudioPlayer::~AudioPlayer() {
  stop();

  // Cleanup audio unit
  if (audioUnit) {
    AudioUnitUninitialize(audioUnit);
    AudioComponentInstanceDispose(audioUnit);
  }
}

OSStatus AudioPlayer::RenderCallback(void *inRefCon,
                                     AudioUnitRenderActionFlags *ioActionFlags,
                                     const AudioTimeStamp *inTimeStamp,
                                     UInt32 inBusNumber, UInt32 inNumberFrames,
                                     AudioBufferList *ioData) {

  AudioPlayer *player = static_cast<AudioPlayer *>(inRefCon);

  // Get buffer
  float *buffer = (float *)ioData->mBuffers[0].mData;
  int channels = player->header.numChannels;
  int bytesPerSample = player->header.bitsPerSample / 8;

  if (!player->playing.load() || player->audioData.empty()) {
    // Fill with silence if not playing
    for (UInt32 i = 0; i < inNumberFrames * channels; i++) {
      buffer[i] = 0.0f;
    }
    return noErr;
  }

  unsigned int position = player->currentPosition.load();
  unsigned int bytesPerFrame = channels * bytesPerSample;
  unsigned int bytesNeeded = inNumberFrames * bytesPerFrame;

  // Check if we have enough data left
  if (position + bytesNeeded > player->audioData.size()) {
    bytesNeeded = player->audioData.size() - position;

    // We'll reach the end of the file during this callback
    UInt32 framesToFill = bytesNeeded / bytesPerFrame;

    // Fill the first part with actual audio data
    for (UInt32 i = 0; i < framesToFill; i++) {
      for (int ch = 0; ch < channels; ch++) {
        int sampleIndex =
            position + (i * bytesPerFrame) + (ch * bytesPerSample);

        // Convert based on bits per sample
        if (bytesPerSample == 2) { // 16-bit
          int16_t sample =
              *reinterpret_cast<int16_t *>(&player->audioData[sampleIndex]);
          buffer[i * channels + ch] = sample / 32768.0f;
        } else if (bytesPerSample == 1) { // 8-bit
          uint8_t sample = player->audioData[sampleIndex];
          buffer[i * channels + ch] = (sample - 128) / 128.0f;
        } else if (bytesPerSample == 4) { // 32-bit
          int32_t sample =
              *reinterpret_cast<int32_t *>(&player->audioData[sampleIndex]);
          buffer[i * channels + ch] = sample / 2147483648.0f;
        }
      }
    }

    // Fill the rest with silence
    for (UInt32 i = framesToFill; i < inNumberFrames; i++) {
      for (int ch = 0; ch < channels; ch++) {
        buffer[i * channels + ch] = 0.0f;
      }
    }

    // Reset position or stop playback
    player->currentPosition.store(0);
    player->playing.store(false);
  } else {
    // Normal case: convert and copy all requested frames
    for (UInt32 i = 0; i < inNumberFrames; i++) {
      for (int ch = 0; ch < channels; ch++) {
        int sampleIndex =
            position + (i * bytesPerFrame) + (ch * bytesPerSample);

        // Convert based on bits per sample
        if (bytesPerSample == 2) { // 16-bit
          int16_t sample =
              *reinterpret_cast<int16_t *>(&player->audioData[sampleIndex]);
          buffer[i * channels + ch] = sample / 32768.0f;
        } else if (bytesPerSample == 1) { // 8-bit
          uint8_t sample = player->audioData[sampleIndex];
          buffer[i * channels + ch] = (sample - 128) / 128.0f;
        } else if (bytesPerSample == 4) { // 32-bit
          int32_t sample =
              *reinterpret_cast<int32_t *>(&player->audioData[sampleIndex]);
          buffer[i * channels + ch] = sample / 2147483648.0f;
        }
      }
    }

    // Update position
    player->currentPosition.store(position + bytesNeeded);
  }

  return noErr;
}

bool AudioPlayer::setupAudioUnit() {
  OSStatus status;

  // Set up the audio component description
  AudioComponentDescription desc;
  desc.componentType = kAudioUnitType_Output;
  desc.componentSubType = kAudioUnitSubType_DefaultOutput;
  desc.componentManufacturer = kAudioUnitManufacturer_Apple;
  desc.componentFlags = 0;
  desc.componentFlagsMask = 0;

  // Find a component that matches the description
  AudioComponent component = AudioComponentFindNext(NULL, &desc);
  if (component == NULL) {
    std::cerr << "Error: Could not find audio component" << std::endl;
    return false;
  }

  // Create an instance of the audio unit
  status = AudioComponentInstanceNew(component, &audioUnit);
  if (status != noErr) {
    std::cerr << "Error: Could not create audio unit instance: " << status
              << std::endl;
    return false;
  }

  // Set up the format of the audio we'll be playing
  audioFormat.mSampleRate = header.sampleRate;
  audioFormat.mFormatID = kAudioFormatLinearPCM;
  audioFormat.mFormatFlags = kAudioFormatFlagIsFloat | kAudioFormatFlagIsPacked;
  audioFormat.mFramesPerPacket = 1;
  audioFormat.mChannelsPerFrame = header.numChannels;
  audioFormat.mBitsPerChannel = 32; // Always use 32-bit float for output
  audioFormat.mBytesPerFrame =
      audioFormat.mChannelsPerFrame * (audioFormat.mBitsPerChannel / 8);
  audioFormat.mBytesPerPacket =
      audioFormat.mBytesPerFrame * audioFormat.mFramesPerPacket;

  // Set the audio unit's input format
  status = AudioUnitSetProperty(audioUnit, kAudioUnitProperty_StreamFormat,
                                kAudioUnitScope_Input, 0, &audioFormat,
                                sizeof(audioFormat));
  if (status != noErr) {
    std::cerr << "Error: Could not set audio unit format: " << status
              << std::endl;
    AudioComponentInstanceDispose(audioUnit);
    return false;
  }

  // Set up the render callback
  AURenderCallbackStruct callbackStruct;
  callbackStruct.inputProc = RenderCallback;
  callbackStruct.inputProcRefCon = this;

  status = AudioUnitSetProperty(audioUnit, kAudioUnitProperty_SetRenderCallback,
                                kAudioUnitScope_Input, 0, &callbackStruct,
                                sizeof(callbackStruct));
  if (status != noErr) {
    std::cerr << "Error: Could not set render callback: " << status
              << std::endl;
    AudioComponentInstanceDispose(audioUnit);
    return false;
  }

  // Initialize the audio unit
  status = AudioUnitInitialize(audioUnit);
  if (status != noErr) {
    std::cerr << "Error: Could not initialize audio unit: " << status
              << std::endl;
    AudioComponentInstanceDispose(audioUnit);
    return false;
  }

  return true;
}

bool AudioPlayer::initialize(const WavHeader &wavHeader) {
  header = wavHeader;
  currentPosition.store(0);

  // Print audio details
  std::cout << "Audio details:" << std::endl;
  std::cout << "Channels: " << header.numChannels << std::endl;
  std::cout << "Sample rate: " << header.sampleRate << " Hz" << std::endl;
  std::cout << "Bits per sample: " << header.bitsPerSample << std::endl;

  return setupAudioUnit();
}

void AudioPlayer::addAudioData(const std::vector<char> &data) {
  // Append the new data to our audio buffer
  audioData.insert(audioData.end(), data.begin(), data.end());
}

void AudioPlayer::clearAudioData() {
  stop();
  audioData.clear();
  currentPosition.store(0);
}

bool AudioPlayer::play() {
  if (audioData.empty()) {
    std::cerr << "Error: No audio data loaded" << std::endl;
    return false;
  }

  OSStatus status = AudioOutputUnitStart(audioUnit);
  if (status != noErr) {
    std::cerr << "Error: Could not start audio unit: " << status << std::endl;
    return false;
  }

  playing.store(true);
  std::cout << "Playing audio" << std::endl;
  return true;
}

bool AudioPlayer::stop() {
  playing.store(false);
  currentPosition.store(0);

  OSStatus status = AudioOutputUnitStop(audioUnit);
  if (status != noErr) {
    std::cerr << "Error: Could not stop audio unit: " << status << std::endl;
    return false;
  }

  std::cout << "Playback stopped." << std::endl;
  return true;
}

bool AudioPlayer::pause() {
  playing.store(false);

  std::cout << "Playback paused at position: " << getPositionInSeconds()
            << " seconds" << std::endl;
  return true;
}

bool AudioPlayer::seekToPosition(double seconds) {
  if (audioData.empty()) {
    std::cerr << "Error: No audio data loaded" << std::endl;
    return false;
  }

  // Calculate position in bytes
  int bytesPerSecond =
      header.sampleRate * header.numChannels * (header.bitsPerSample / 8);
  unsigned int position = static_cast<unsigned int>(seconds * bytesPerSecond);

  // Align to frame boundary
  int bytesPerFrame = header.numChannels * (header.bitsPerSample / 8);
  position = (position / bytesPerFrame) * bytesPerFrame;

  if (position >= audioData.size()) {
    std::cerr << "Error: Position is beyond the end of the file" << std::endl;
    return false;
  }

  currentPosition.store(position);

  std::cout << "Seeked to position: " << seconds << " seconds" << std::endl;
  return true;
}

double AudioPlayer::getPositionInSeconds() const {
  if (audioData.empty()) {
    return 0.0;
  }

  int bytesPerSecond =
      header.sampleRate * header.numChannels * (header.bitsPerSample / 8);
  return static_cast<double>(currentPosition.load()) / bytesPerSecond;
}

double AudioPlayer::getDurationInSeconds() const {
  if (audioData.empty()) {
    return 0.0;
  }

  int bytesPerSecond =
      header.sampleRate * header.numChannels * (header.bitsPerSample / 8);
  return static_cast<double>(audioData.size()) / bytesPerSecond;
}

bool AudioPlayer::isPlaying() const { return playing.load(); }

bool AudioPlayer::setSyncTimestamp(uint64_t timestamp) {
  syncTimestamp.store(timestamp);
  return true;
}

uint64_t AudioPlayer::getSyncTimestamp() const { return syncTimestamp.load(); }

bool AudioPlayer::syncWithTimestamp(uint64_t timestamp,
                                    double positionInSeconds) {
  if (audioData.empty()) {
    std::cerr << "Error: No audio data loaded" << std::endl;
    return false;
  }

  // Calculate the time difference
  uint64_t currentTime =
      std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::system_clock::now().time_since_epoch())
          .count();
  int64_t timeDiff = timestamp - currentTime;

  // If the timestamp is in the future, wait
  if (timeDiff > 0) {
    std::cout << "Waiting " << timeDiff << " ms for sync..." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(timeDiff));
  }

  // Seek to the specified position
  seekToPosition(positionInSeconds);

  // Start playing
  play();

  return true;
}
