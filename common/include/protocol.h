#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <cstdint>
#include <string>
#include <vector>
#include "wav_header.h"

// Message types for client-server communication
enum class MessageType : uint8_t {
  LIST_REQUEST,         // Client requests list of available songs
  LIST_RESPONSE,        // Server responds with list of songs
  SONG_REQUEST,         // Client requests a specific song
  SONG_INFO,            // Server sends song info (header)
  SONG_DATA,            // Server sends song data chunks
  SONG_DATA_END,        // Server indicates end of song data
  PLAY_CONTROL,         // Client sends play control commands (play, pause, etc.)
  ERROR                 // Error message
};

// Play control commands
enum class PlayControl : uint8_t {
  PLAY,
  PAUSE,
  STOP,
  SEEK
};

// Message header structure
struct MessageHeader {
  MessageType type;
  uint32_t size;  // Size of the message payload in bytes
};

// Control message structure for play/pause/seek commands
struct ControlMessage {
  PlayControl command;
  union {
    double seekPosition;  // In seconds, used for SEEK command
    uint8_t padding[8];   // Ensure consistent size
  };
};

// Function to serialize messages for network transmission
template<typename T>
std::vector<char> serializeMessage(MessageType type, const T& data) {
  std::vector<char> buffer;
  MessageHeader header{type, sizeof(T)};
  
  // Add header
  buffer.resize(sizeof(MessageHeader));
  memcpy(buffer.data(), &header, sizeof(MessageHeader));
  
  // Add data
  size_t offset = sizeof(MessageHeader);
  buffer.resize(offset + sizeof(T));
  memcpy(buffer.data() + offset, &data, sizeof(T));
  
  return buffer;
}

// Specialization for string data (e.g., song names)
template<>
inline std::vector<char> serializeMessage<std::string>(MessageType type, const std::string& data) {
  std::vector<char> buffer;
  MessageHeader header{type, static_cast<uint32_t>(data.size())};
  
  // Add header
  buffer.resize(sizeof(MessageHeader));
  memcpy(buffer.data(), &header, sizeof(MessageHeader));
  
  // Add string data
  size_t offset = sizeof(MessageHeader);
  buffer.resize(offset + data.size());
  memcpy(buffer.data() + offset, data.c_str(), data.size());
  
  return buffer;
}

// Specialization for vector<string> (e.g., song list)
template<>
inline std::vector<char> serializeMessage<std::vector<std::string>>(MessageType type, const std::vector<std::string>& data) {
  std::vector<char> buffer;
  
  // Calculate total size
  uint32_t totalSize = 4;  // 4 bytes for the count
  for (const auto& str : data) {
    totalSize += 4 + static_cast<uint32_t>(str.size());  // 4 bytes for string length + string data
  }
  
  MessageHeader header{type, totalSize};
  
  // Add header
  buffer.resize(sizeof(MessageHeader));
  memcpy(buffer.data(), &header, sizeof(MessageHeader));
  
  // Add vector size
  size_t offset = sizeof(MessageHeader);
  buffer.resize(offset + 4);
  uint32_t count = static_cast<uint32_t>(data.size());
  memcpy(buffer.data() + offset, &count, 4);
  offset += 4;
  
  // Add strings
  for (const auto& str : data) {
    // Add string length
    buffer.resize(offset + 4);
    uint32_t length = static_cast<uint32_t>(str.size());
    memcpy(buffer.data() + offset, &length, 4);
    offset += 4;
    
    // Add string data
    buffer.resize(offset + str.size());
    memcpy(buffer.data() + offset, str.c_str(), str.size());
    offset += str.size();
  }
  
  return buffer;
}

// Specialization for WavHeader
template<>
inline std::vector<char> serializeMessage<WavHeader>(MessageType type, const WavHeader& data) {
  std::vector<char> buffer;
  MessageHeader header{type, sizeof(WavHeader)};
  
  // Add header
  buffer.resize(sizeof(MessageHeader));
  memcpy(buffer.data(), &header, sizeof(MessageHeader));
  
  // Add WavHeader data
  size_t offset = sizeof(MessageHeader);
  buffer.resize(offset + sizeof(WavHeader));
  memcpy(buffer.data() + offset, &data, sizeof(WavHeader));
  
  return buffer;
}

// Specialization for raw audio data chunks
inline std::vector<char> serializeAudioData(const std::vector<char>& data, size_t offset, size_t chunkSize) {
  std::vector<char> buffer;
  size_t actualSize = std::min(chunkSize, data.size() - offset);
  MessageHeader header{MessageType::SONG_DATA, static_cast<uint32_t>(actualSize)};
  
  // Add header
  buffer.resize(sizeof(MessageHeader));
  memcpy(buffer.data(), &header, sizeof(MessageHeader));
  
  // Add audio data
  buffer.resize(sizeof(MessageHeader) + actualSize);
  memcpy(buffer.data() + sizeof(MessageHeader), data.data() + offset, actualSize);
  
  return buffer;
}

#endif // PROTOCOL_H