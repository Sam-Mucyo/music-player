#ifndef WAV_FILE_H
#define WAV_FILE_H

#include <string>
#include <vector>
#include "../../common/include/wav_header.h"

class WavFile {
private:
    std::string filepath;
    WavHeader header;
    std::vector<char> audioData;
    
    bool readWavFile();
    
public:
    WavFile(const std::string& path);
    ~WavFile();
    
    bool load();
    bool isLoaded() const;
    
    const WavHeader& getHeader() const;
    const std::vector<char>& getAudioData() const;
    const std::string& getFilePath() const;
    
    double getDurationInSeconds() const;
};

#endif // WAV_FILE_H