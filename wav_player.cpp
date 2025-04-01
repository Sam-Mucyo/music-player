#include <atomic>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <fstream>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

// macOS specific includes
#include <AudioToolbox/AudioToolbox.h>
#include <CoreAudio/CoreAudio.h>
#include <sys/stat.h>
#include <unistd.h>

// WAV file header structure
struct WavHeader {
  char riff[4];               // "RIFF"
  unsigned int fileSize;      // Total file size minus 8 bytes
  char wave[4];               // "WAVE"
  char fmt[4];                // "fmt "
  unsigned int fmtSize;       // Size of format chunk (16 for PCM)
  unsigned short audioFormat; // Audio format (1 for PCM)
  unsigned short numChannels; // Number of channels
  unsigned int sampleRate;    // Sample rate
  unsigned int
      byteRate; // Byte rate (SampleRate * NumChannels * BitsPerSample/8)
  unsigned short blockAlign;    // Block align (NumChannels * BitsPerSample/8)
  unsigned short bitsPerSample; // Bits per sample
  char data[4];                 // "data"
  unsigned int dataSize;        // Size of data chunk
};

class WavPlayer {
private:
  std::string filepath;
  FILE *wavFile;
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
                                 AudioBufferList *ioData) {

    WavPlayer *player = static_cast<WavPlayer *>(inRefCon);

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

  bool readWavFile() {
    wavFile = fopen(filepath.c_str(), "rb");
    if (!wavFile) {
      std::cerr << "Error: Cannot open file " << filepath << std::endl;
      return false;
    }

    // Read the header
    if (fread(&header, sizeof(WavHeader), 1, wavFile) != 1) {
      std::cerr << "Error: Cannot read WAV header" << std::endl;
      fclose(wavFile);
      return false;
    }

    // Validate the header
    if (strncmp(header.riff, "RIFF", 4) != 0 ||
        strncmp(header.wave, "WAVE", 4) != 0 ||
        strncmp(header.fmt, "fmt ", 4) != 0) {
      std::cerr << "Error: Invalid WAV format" << std::endl;
      fclose(wavFile);
      return false;
    }

    // Find data chunk (some WAV files have extra chunks)
    bool dataChunkFound = false;
    unsigned int dataChunkSize = 0;

    // If the header doesn't match a standard WAV header, we need to search for
    // the data chunk
    if (strncmp(header.data, "data", 4) != 0) {
      char chunkId[4];
      unsigned int chunkSize;

      // Skip past the format chunk
      fseek(wavFile, 12 + 8 + header.fmtSize, SEEK_SET);

      // Search for the data chunk
      while (fread(chunkId, 1, 4, wavFile) == 4) {
        fread(&chunkSize, 4, 1, wavFile);

        if (strncmp(chunkId, "data", 4) == 0) {
          dataChunkSize = chunkSize;
          dataChunkFound = true;
          break;
        }

        // Skip to the next chunk
        fseek(wavFile, chunkSize, SEEK_CUR);
      }
    } else {
      dataChunkFound = true;
      dataChunkSize = header.dataSize;
    }

    if (!dataChunkFound) {
      std::cerr << "Error: Could not find data chunk in WAV file" << std::endl;
      fclose(wavFile);
      return false;
    }

    // Read the audio data
    audioData.resize(dataChunkSize);
    if (fread(audioData.data(), 1, dataChunkSize, wavFile) != dataChunkSize) {
      std::cerr << "Error: Could not read the entire audio data" << std::endl;
      fclose(wavFile);
      return false;
    }

    fclose(wavFile);
    wavFile = nullptr;

    std::cout << "WAV file details:" << std::endl;
    std::cout << "Channels: " << header.numChannels << std::endl;
    std::cout << "Sample rate: " << header.sampleRate << " Hz" << std::endl;
    std::cout << "Bits per sample: " << header.bitsPerSample << std::endl;
    std::cout << "Duration: "
              << static_cast<double>(dataChunkSize) /
                     (header.sampleRate * header.numChannels *
                      (header.bitsPerSample / 8))
              << " seconds" << std::endl;

    return true;
  }

  bool setupAudioUnit() {
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
    audioFormat.mFormatFlags =
        kAudioFormatFlagIsFloat | kAudioFormatFlagIsPacked;
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

    status = AudioUnitSetProperty(
        audioUnit, kAudioUnitProperty_SetRenderCallback, kAudioUnitScope_Input,
        0, &callbackStruct, sizeof(callbackStruct));
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

public:
  WavPlayer(const std::string &path)
      : filepath(path), wavFile(nullptr), playing(false), shouldStop(false),
        currentPosition(0), syncTimestamp(0) {}

  ~WavPlayer() {
    stop();

    // Cleanup audio unit
    if (audioUnit) {
      AudioUnitUninitialize(audioUnit);
      AudioComponentInstanceDispose(audioUnit);
    }

    if (wavFile) {
      fclose(wavFile);
    }
  }

  bool fileExists() const {
    struct stat buffer;
    return (stat(filepath.c_str(), &buffer) == 0);
  }

  bool initialize() {
    if (!fileExists()) {
      std::cerr << "Error: File does not exist: " << filepath << std::endl;
      return false;
    }

    if (!readWavFile()) {
      return false;
    }

    if (!setupAudioUnit()) {
      return false;
    }

    return true;
  }

  bool play() {
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
    std::cout << "Playing: " << filepath << std::endl;
    return true;
  }

  bool stop() {
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

  bool pause() {
    playing.store(false);

    std::cout << "Playback paused at position: " << getPositionInSeconds()
              << " seconds" << std::endl;
    return true;
  }

  bool seekToPosition(double seconds) {
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

  double getPositionInSeconds() const {
    if (audioData.empty()) {
      return 0.0;
    }

    int bytesPerSecond =
        header.sampleRate * header.numChannels * (header.bitsPerSample / 8);
    return static_cast<double>(currentPosition.load()) / bytesPerSecond;
  }

  double getDurationInSeconds() const {
    if (audioData.empty()) {
      return 0.0;
    }

    int bytesPerSecond =
        header.sampleRate * header.numChannels * (header.bitsPerSample / 8);
    return static_cast<double>(audioData.size()) / bytesPerSecond;
  }

  // Network synchronization functions
  bool setSyncTimestamp(uint64_t timestamp) {
    syncTimestamp.store(timestamp);
    return true;
  }

  uint64_t getSyncTimestamp() const { return syncTimestamp.load(); }

  bool syncWithTimestamp(uint64_t timestamp, double positionInSeconds) {
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
};

int main(int argc, char *argv[]) {
  // Check if a file path was provided
  if (argc < 2) {
    std::cout << "Usage: " << argv[0] << " <path_to_wav_file>" << std::endl;
    return 1;
  }

  // Create player with the specified file
  WavPlayer player(argv[1]);

  // Initialize the player
  if (!player.initialize()) {
    return 1;
  }

  // Interactive command loop
  std::string command;
  std::cout << "\nCommands: play, pause, stop, seek <seconds>, position, "
               "duration, sync <timestamp> <position>, exit\n"
            << std::endl;

  while (true) {
    std::cout << "> ";
    std::getline(std::cin, command);

    if (command == "play") {
      player.play();
    } else if (command == "pause") {
      player.pause();
    } else if (command == "stop") {
      player.stop();
    } else if (command.substr(0, 5) == "seek ") {
      try {
        double seconds = std::stod(command.substr(5));
        player.seekToPosition(seconds);
      } catch (...) {
        std::cerr << "Invalid position. Usage: seek <seconds>" << std::endl;
      }
    } else if (command == "position") {
      std::cout << "Current position: " << player.getPositionInSeconds()
                << " seconds" << std::endl;
    } else if (command == "duration") {
      std::cout << "File duration: " << player.getDurationInSeconds()
                << " seconds" << std::endl;
    } else if (command.substr(0, 5) == "sync ") {
      try {
        size_t spacePos = command.find(' ', 5);
        if (spacePos != std::string::npos) {
          uint64_t timestamp = std::stoull(command.substr(5, spacePos - 5));
          double position = std::stod(command.substr(spacePos + 1));
          player.syncWithTimestamp(timestamp, position);
        } else {
          std::cerr
              << "Invalid sync command. Usage: sync <timestamp> <position>"
              << std::endl;
        }
      } catch (...) {
        std::cerr
            << "Invalid sync parameters. Usage: sync <timestamp> <position>"
            << std::endl;
      }
    } else if (command == "exit") {
      break;
    } else {
      std::cout
          << "Unknown command. Available commands: play, pause, stop, seek "
             "<seconds>, position, duration, sync <timestamp> <position>, exit"
          << std::endl;
    }
  }

  return 0;
}
