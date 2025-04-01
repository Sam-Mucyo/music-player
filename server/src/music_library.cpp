#include "music_library.h"
#include <dirent.h>
#include <algorithm>
#include <iostream>
#include <filesystem>

MusicLibrary::MusicLibrary(const std::string& directory) : musicDir(directory) {
    scanMusicDirectory();
}

MusicLibrary::~MusicLibrary() {
}

void MusicLibrary::scanMusicDirectory() {
    DIR* dir;
    struct dirent* ent;
    
    if ((dir = opendir(musicDir.c_str())) != nullptr) {
        songNames.clear();
        
        while ((ent = readdir(dir)) != nullptr) {
            std::string filename = ent->d_name;
            
            // Check if this is a .wav file
            if (filename.size() > 4 && 
                filename.substr(filename.size() - 4) == ".wav") {
                
                // Add the song to our list (without the .wav extension)
                songNames.push_back(filename);
            }
        }
        
        closedir(dir);
        
        // Sort the songs alphabetically
        std::sort(songNames.begin(), songNames.end());
        
        std::cout << "Found " << songNames.size() << " songs in " << musicDir << std::endl;
    } else {
        std::cerr << "Could not open directory: " << musicDir << std::endl;
    }
}

const std::vector<std::string>& MusicLibrary::getSongList() const {
    return songNames;
}

std::shared_ptr<WavFile> MusicLibrary::getSong(const std::string& songName) {
    // Check if the song is already loaded
    auto it = loadedSongs.find(songName);
    if (it != loadedSongs.end()) {
        return it->second;
    }
    
    // Load the song
    std::string filepath = musicDir + "/" + songName;
    auto song = std::make_shared<WavFile>(filepath);
    
    if (song->load()) {
        // Add to cache
        loadedSongs[songName] = song;
        return song;
    }
    
    return nullptr;
}

bool MusicLibrary::hasSong(const std::string& songName) const {
    return std::find(songNames.begin(), songNames.end(), songName) != songNames.end();
}