#include <gtest/gtest.h>
#include "music_library.h"
#include <filesystem>
#include <fstream>

class MusicLibraryTest : public ::testing::Test {
protected:
    std::string testDir;
    
    void SetUp() override {
        // Create a test directory for music files
        testDir = "bin/test_data/music_library_test";
        if (std::filesystem::exists(testDir)) {
            std::filesystem::remove_all(testDir);
        }
        std::filesystem::create_directories(testDir);
        
        // Create a simple test WAV file
        createDummyWavFile(testDir + "/test1.wav");
        createDummyWavFile(testDir + "/test2.wav");
        
        // Create a non-WAV file to ensure it's ignored
        createDummyFile(testDir + "/test.txt");
    }
    
    void TearDown() override {
        if (std::filesystem::exists(testDir)) {
            std::filesystem::remove_all(testDir);
        }
    }
    
    void createDummyWavFile(const std::string& path) {
        std::ofstream file(path, std::ios::binary);
        // Create a minimal valid WAV header
        const char wavHeader[] = {
            'R', 'I', 'F', 'F',             // RIFF header
            44, 0, 0, 0,                    // Chunk size (36 + 8 bytes of data)
            'W', 'A', 'V', 'E',             // WAVE header
            'f', 'm', 't', ' ',             // fmt header
            16, 0, 0, 0,                    // fmt chunk size
            1, 0,                           // Audio format (PCM)
            1, 0,                           // Num channels (Mono)
            0x44, 0xAC, 0, 0,               // Sample rate (44100)
            0x44, 0xAC, 0, 0,               // Byte rate (44100)
            1, 0,                           // Block align
            8, 0,                           // Bits per sample
            'd', 'a', 't', 'a',             // data header
            8, 0, 0, 0,                     // Data chunk size (8 bytes)
            0, 0, 0, 0, 0, 0, 0, 0          // 8 bytes of silence
        };
        file.write(wavHeader, sizeof(wavHeader));
        file.close();
    }
    
    void createDummyFile(const std::string& path) {
        std::ofstream file(path);
        file << "This is not a WAV file";
        file.close();
    }
};

TEST_F(MusicLibraryTest, LoadDirectory) {
    MusicLibrary library;
    library.loadDirectory(testDir);
    
    // Should find exactly 2 WAV files
    auto songList = library.getSongList();
    EXPECT_EQ(songList.size(), 2);
    
    // Verify the songs are loaded correctly
    EXPECT_TRUE(library.hasSong(0));
    EXPECT_TRUE(library.hasSong(1));
    EXPECT_FALSE(library.hasSong(2));  // Beyond the end
    
    // Song paths should end with .wav
    for (const auto& song : songList) {
        EXPECT_TRUE(song.ends_with(".wav"));
    }
}

TEST_F(MusicLibraryTest, GetSong) {
    MusicLibrary library;
    library.loadDirectory(testDir);
    
    // Test getting valid songs
    WavFile* song0 = library.getSong(0);
    EXPECT_NE(song0, nullptr);
    
    WavFile* song1 = library.getSong(1);
    EXPECT_NE(song1, nullptr);
    
    // Test getting invalid song index
    WavFile* invalidSong = library.getSong(100);
    EXPECT_EQ(invalidSong, nullptr);
}

TEST_F(MusicLibraryTest, ReloadDirectory) {
    MusicLibrary library;
    library.loadDirectory(testDir);
    
    // Initially has 2 songs
    EXPECT_EQ(library.getSongList().size(), 2);
    
    // Add a new WAV file to the directory
    createDummyWavFile(testDir + "/test3.wav");
    
    // Reload the directory
    library.loadDirectory(testDir);
    
    // Should now have 3 songs
    EXPECT_EQ(library.getSongList().size(), 3);
    EXPECT_TRUE(library.hasSong(2));
}

TEST_F(MusicLibraryTest, EmptyDirectory) {
    // Create an empty directory
    std::string emptyDir = testDir + "/empty";
    std::filesystem::create_directories(emptyDir);
    
    MusicLibrary library;
    library.loadDirectory(emptyDir);
    
    // Should have no songs
    EXPECT_EQ(library.getSongList().size(), 0);
    EXPECT_FALSE(library.hasSong(0));
}

TEST_F(MusicLibraryTest, NonExistentDirectory) {
    MusicLibrary library;
    library.loadDirectory("non_existent_directory");
    
    // Should have no songs and not crash
    EXPECT_EQ(library.getSongList().size(), 0);
}