#ifndef CLIENT_HANDLER_H
#define CLIENT_HANDLER_H

#include <atomic>
#include <memory>
#include <string>
#include <thread>
#include "../../common/include/socket.h"
#include "music_library.h"

class ClientHandler {
private:
    std::unique_ptr<Socket> clientSocket;
    std::shared_ptr<MusicLibrary> library;
    std::atomic<bool> isRunning;
    std::thread clientThread;
    
    // Handle client request
    void handleClient();
    
    // Send the list of available songs to the client
    bool sendSongList();
    
    // Send a song to the client
    bool sendSong(const std::string& songName);
    
    // Send an error message to the client
    bool sendError(const std::string& errorMessage);

public:
    ClientHandler(std::unique_ptr<Socket> socket, std::shared_ptr<MusicLibrary> musicLibrary);
    ~ClientHandler();
    
    // Start handling the client
    void start();
    
    // Stop handling the client
    void stop();
    
    // Check if the handler is running
    bool isActive() const;
};

#endif // CLIENT_HANDLER_H