#include <gtest/gtest.h>
#include "wav_file.h"
#include <string>
#include <filesystem>

class WavFileTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Make sure test files are available
        std::string testDataPath = "bin/test_data";
        if (!std::filesystem::exists(testDataPath)) {
            std::filesystem::create_directories(testDataPath);
        }
    }
};

TEST_F(WavFileTest, ConstructorInitialization) {
    WavFile wavFile;
    // Test initial state
    EXPECT_EQ(wavFile.getFilePath(), "");
    EXPECT_EQ(wavFile.getDuration(), 0.0f);
}

TEST_F(WavFileTest, NonExistentFileHandling) {
    WavFile wavFile;
    bool result = wavFile.load("non_existent_file.wav");
    
    // Should return false for non-existent file
    EXPECT_FALSE(result);
    EXPECT_EQ(wavFile.getDuration(), 0.0f);
}

// This test assumes access to a valid WAV file
// In a real test suite, you'd want to include a small test WAV file in your repo
TEST_F(WavFileTest, LoadRealFile) {
    // This test is conditionally executed only if the sample file exists
    std::string samplePath = "bin/music/Synth 108 Bm 2.wav";
    if (!std::filesystem::exists(samplePath)) {
        GTEST_SKIP() << "Sample WAV file not found";
    }
    
    WavFile wavFile;
    bool result = wavFile.load(samplePath);
    
    EXPECT_TRUE(result);
    EXPECT_GT(wavFile.getDuration(), 0.0f);
    
    // A valid WAV file should have audio data
    const char* audioData = wavFile.getAudioData();
    EXPECT_NE(audioData, nullptr);
    
    // Check header fields for reasonable values
    const WavHeader& header = wavFile.getHeader();
    EXPECT_EQ(header.riffHeader, 0x46464952); // "RIFF" in little endian
    EXPECT_EQ(header.waveHeader, 0x45564157); // "WAVE" in little endian
    EXPECT_GT(header.subchunk2Size, 0u);      // Data size should be greater than 0
    
    // Ensure we recognize the format
    EXPECT_TRUE(header.audioFormat == 1 || header.audioFormat == 3); // PCM or IEEE float
    EXPECT_TRUE(header.bitsPerSample == 8 || header.bitsPerSample == 16 || 
                header.bitsPerSample == 24 || header.bitsPerSample == 32);
}