#include "wav_file.h"
#include <cstring>
#include <iostream>

WavFile::WavFile(const std::string& path) : filepath(path) {
}

WavFile::~WavFile() {
}

bool WavFile::load() {
    return readWavFile();
}

bool WavFile::readWavFile() {
    FILE* wavFile = fopen(filepath.c_str(), "rb");
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
    
    std::cout << "Loaded WAV file: " << filepath << std::endl;
    std::cout << "Channels: " << header.numChannels << std::endl;
    std::cout << "Sample rate: " << header.sampleRate << " Hz" << std::endl;
    std::cout << "Bits per sample: " << header.bitsPerSample << std::endl;
    std::cout << "Duration: " 
              << static_cast<double>(dataChunkSize) / 
                 (header.sampleRate * header.numChannels * (header.bitsPerSample / 8))
              << " seconds" << std::endl;
    
    return true;
}

bool WavFile::isLoaded() const {
    return !audioData.empty();
}

const WavHeader& WavFile::getHeader() const {
    return header;
}

const std::vector<char>& WavFile::getAudioData() const {
    return audioData;
}

const std::string& WavFile::getFilePath() const {
    return filepath;
}

double WavFile::getDurationInSeconds() const {
    if (audioData.empty()) {
        return 0.0;
    }
    
    int bytesPerSecond = 
        header.sampleRate * header.numChannels * (header.bitsPerSample / 8);
    return static_cast<double>(audioData.size()) / bytesPerSecond;
}