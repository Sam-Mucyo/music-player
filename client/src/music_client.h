#ifndef MUSIC_CLIENT_H
#define MUSIC_CLIENT_H

#include <atomic>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "../../common/include/socket.h"
#include "../../common/include/protocol.h"
#include "audio_player.h"

class MusicClient {
private:
    std::unique_ptr<Socket> socket;
    std::unique_ptr<AudioPlayer> player;
    std::atomic<bool> isRunning;
    std::thread receiveThread;
    std::string currentSong;
    std::vector<std::string> availableSongs;
    
    // Buffer for receiving audio data
    std::vector<char> audioBuffer;
    bool isBuffering;
    
    // Handle received messages
    void handleMessage(const MessageHeader& header, const std::vector<char>& data);
    
    // Receive thread function
    void receiveThreadFunc();
    
    // Utility functions
    std::vector<std::string> parseStringList(const std::vector<char>& data);

public:
    MusicClient();
    ~MusicClient();
    
    // Connect to server
    bool connect(const std::string& host, int port);
    
    // Disconnect from server
    void disconnect();
    
    // Request the list of available songs
    bool requestSongList();
    
    // Request a specific song
    bool requestSong(const std::string& songName);
    
    // Playback control functions
    bool play();
    bool pause();
    bool stop();
    bool seek(double position);
    
    // Status functions
    double getCurrentPosition() const;
    double getDuration() const;
    bool isPlaying() const;
    std::string getCurrentSong() const;
    const std::vector<std::string>& getAvailableSongs() const;
    bool isConnected() const;
};

#endif // MUSIC_CLIENT_H