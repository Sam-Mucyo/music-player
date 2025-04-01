#include <gtest/gtest.h>
#include "protocol.h"
#include <vector>
#include <string>

class ProtocolTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup code
    }
};

TEST_F(ProtocolTest, StringSerializationDeserialization) {
    // Test serializing a string
    std::string testString = "Hello, World!";
    std::vector<char> serialized = serializeMessage<std::string>(MessageType::SONG_REQUEST, testString);
    
    // Verify header
    ASSERT_GE(serialized.size(), sizeof(MessageHeader));
    MessageHeader* header = reinterpret_cast<MessageHeader*>(serialized.data());
    EXPECT_EQ(header->type, MessageType::SONG_REQUEST);
    EXPECT_EQ(header->size, testString.size());
    
    // Verify payload
    std::string deserializedString(serialized.data() + sizeof(MessageHeader), 
                                   serialized.data() + sizeof(MessageHeader) + header->size);
    EXPECT_EQ(deserializedString, testString);
}

TEST_F(ProtocolTest, StringVectorSerializationDeserialization) {
    // Test serializing a vector of strings
    std::vector<std::string> testStrings = {"First song", "Second song", "Third song"};
    std::vector<char> serialized = serializeMessage<std::vector<std::string>>(
        MessageType::LIST_RESPONSE, testStrings);
    
    // Verify header
    ASSERT_GE(serialized.size(), sizeof(MessageHeader));
    MessageHeader* header = reinterpret_cast<MessageHeader*>(serialized.data());
    EXPECT_EQ(header->type, MessageType::LIST_RESPONSE);
    
    // Calculate expected size
    size_t expectedSize = 0;
    for (const auto& str : testStrings) {
        expectedSize += sizeof(uint32_t) + str.size();
    }
    EXPECT_EQ(header->size, expectedSize);
    
    // Manual deserialization to verify contents
    const char* data = serialized.data() + sizeof(MessageHeader);
    std::vector<std::string> deserializedStrings;
    
    size_t offset = 0;
    while (offset < header->size) {
        uint32_t strSize = *reinterpret_cast<const uint32_t*>(data + offset);
        offset += sizeof(uint32_t);
        
        std::string str(data + offset, data + offset + strSize);
        deserializedStrings.push_back(str);
        offset += strSize;
    }
    
    ASSERT_EQ(deserializedStrings.size(), testStrings.size());
    for (size_t i = 0; i < testStrings.size(); i++) {
        EXPECT_EQ(deserializedStrings[i], testStrings[i]);
    }
}

TEST_F(ProtocolTest, ControlMessageSerialization) {
    // Test play control message
    ControlMessage playMsg;
    playMsg.command = PlayControl::PLAY;
    playMsg.seekPosition = 0;  // Not used for PLAY
    
    std::vector<char> serialized = serializeMessage<ControlMessage>(
        MessageType::PLAY_CONTROL, playMsg);
    
    // Verify header
    ASSERT_GE(serialized.size(), sizeof(MessageHeader));
    MessageHeader* header = reinterpret_cast<MessageHeader*>(serialized.data());
    EXPECT_EQ(header->type, MessageType::PLAY_CONTROL);
    EXPECT_EQ(header->size, sizeof(ControlMessage));
    
    // Verify payload
    ControlMessage* deserializedMsg = reinterpret_cast<ControlMessage*>(
        serialized.data() + sizeof(MessageHeader));
    EXPECT_EQ(deserializedMsg->command, PlayControl::PLAY);
    
    // Test seek control message
    ControlMessage seekMsg;
    seekMsg.command = PlayControl::SEEK;
    seekMsg.seekPosition = 30.5f;  // Seek to 30.5 seconds
    
    serialized = serializeMessage<ControlMessage>(MessageType::PLAY_CONTROL, seekMsg);
    
    // Verify header
    header = reinterpret_cast<MessageHeader*>(serialized.data());
    EXPECT_EQ(header->type, MessageType::PLAY_CONTROL);
    EXPECT_EQ(header->size, sizeof(ControlMessage));
    
    // Verify payload
    deserializedMsg = reinterpret_cast<ControlMessage*>(
        serialized.data() + sizeof(MessageHeader));
    EXPECT_EQ(deserializedMsg->command, PlayControl::SEEK);
    EXPECT_FLOAT_EQ(deserializedMsg->seekPosition, 30.5f);
}

TEST_F(ProtocolTest, WavHeaderSerialization) {
    // Create a sample WAV header
    WavHeader wavHeader;
    wavHeader.riffHeader = 0x46464952;  // "RIFF" in little endian
    wavHeader.chunkSize = 36 + 1000;    // 1000 bytes of audio data + 36 header bytes
    wavHeader.waveHeader = 0x45564157;  // "WAVE" in little endian
    wavHeader.fmtHeader = 0x20746D66;   // "fmt " in little endian
    wavHeader.subchunk1Size = 16;
    wavHeader.audioFormat = 1;         // PCM
    wavHeader.numChannels = 2;         // Stereo
    wavHeader.sampleRate = 44100;
    wavHeader.byteRate = 176400;       // 44100 * 2 * 2
    wavHeader.blockAlign = 4;          // 2 * 2
    wavHeader.bitsPerSample = 16;
    wavHeader.dataHeader = 0x61746164; // "data" in little endian
    wavHeader.subchunk2Size = 1000;    // 1000 bytes of audio data
    
    std::vector<char> serialized = serializeMessage<WavHeader>(
        MessageType::SONG_INFO, wavHeader);
    
    // Verify header
    ASSERT_GE(serialized.size(), sizeof(MessageHeader));
    MessageHeader* header = reinterpret_cast<MessageHeader*>(serialized.data());
    EXPECT_EQ(header->type, MessageType::SONG_INFO);
    EXPECT_EQ(header->size, sizeof(WavHeader));
    
    // Verify payload
    WavHeader* deserializedHeader = reinterpret_cast<WavHeader*>(
        serialized.data() + sizeof(MessageHeader));
    
    EXPECT_EQ(deserializedHeader->riffHeader, wavHeader.riffHeader);
    EXPECT_EQ(deserializedHeader->chunkSize, wavHeader.chunkSize);
    EXPECT_EQ(deserializedHeader->waveHeader, wavHeader.waveHeader);
    EXPECT_EQ(deserializedHeader->fmtHeader, wavHeader.fmtHeader);
    EXPECT_EQ(deserializedHeader->subchunk1Size, wavHeader.subchunk1Size);
    EXPECT_EQ(deserializedHeader->audioFormat, wavHeader.audioFormat);
    EXPECT_EQ(deserializedHeader->numChannels, wavHeader.numChannels);
    EXPECT_EQ(deserializedHeader->sampleRate, wavHeader.sampleRate);
    EXPECT_EQ(deserializedHeader->byteRate, wavHeader.byteRate);
    EXPECT_EQ(deserializedHeader->blockAlign, wavHeader.blockAlign);
    EXPECT_EQ(deserializedHeader->bitsPerSample, wavHeader.bitsPerSample);
    EXPECT_EQ(deserializedHeader->dataHeader, wavHeader.dataHeader);
    EXPECT_EQ(deserializedHeader->subchunk2Size, wavHeader.subchunk2Size);
}

TEST_F(ProtocolTest, AudioDataSerialization) {
    // Create some sample audio data
    const size_t dataSize = 1024;
    std::vector<char> audioData(dataSize);
    for (size_t i = 0; i < dataSize; i++) {
        audioData[i] = static_cast<char>(i % 256);  // Fill with pattern
    }
    
    // Test serializing audio data with different chunk sizes
    for (size_t offset = 0; offset < dataSize; offset += 256) {
        size_t chunkSize = std::min(256UL, dataSize - offset);
        std::vector<char> serialized = serializeAudioData(
            audioData.data() + offset, chunkSize);
        
        // Verify header
        ASSERT_GE(serialized.size(), sizeof(MessageHeader));
        MessageHeader* header = reinterpret_cast<MessageHeader*>(serialized.data());
        EXPECT_EQ(header->type, MessageType::SONG_DATA);
        EXPECT_EQ(header->size, chunkSize);
        
        // Verify payload
        const char* deserializedData = serialized.data() + sizeof(MessageHeader);
        for (size_t i = 0; i < chunkSize; i++) {
            EXPECT_EQ(deserializedData[i], audioData[offset + i]);
        }
    }
}