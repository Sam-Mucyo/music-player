#ifndef MUSIC_LIBRARY_H
#define MUSIC_LIBRARY_H

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "wav_file.h"

class MusicLibrary {
private:
    std::string musicDir;
    std::vector<std::string> songNames;
    std::unordered_map<std::string, std::shared_ptr<WavFile>> loadedSongs;
    
    // Scan the music directory for available songs
    void scanMusicDirectory();

public:
    MusicLibrary(const std::string& directory);
    ~MusicLibrary();
    
    // Get list of available songs
    const std::vector<std::string>& getSongList() const;
    
    // Load a song by name
    std::shared_ptr<WavFile> getSong(const std::string& songName);
    
    // Check if a song exists
    bool hasSong(const std::string& songName) const;
};

#endif // MUSIC_LIBRARY_H