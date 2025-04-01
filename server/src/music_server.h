#ifndef MUSIC_SERVER_H
#define MUSIC_SERVER_H

#include <atomic>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include "client_handler.h"
#include "music_library.h"

class MusicServer {
private:
    int port;
    std::string musicDir;
    std::unique_ptr<Socket> serverSocket;
    std::shared_ptr<MusicLibrary> library;
    std::atomic<bool> isRunning;
    std::thread acceptThread;
    std::vector<std::unique_ptr<ClientHandler>> clients;
    
    // Thread to accept new clients
    void acceptClients();
    
    // Clean up disconnected clients
    void cleanupClients();

public:
    MusicServer(int port, const std::string& musicDirectory);
    ~MusicServer();
    
    // Start the server
    bool start();
    
    // Stop the server
    void stop();
    
    // Check if the server is running
    bool running() const;
    
    // Get the number of connected clients
    size_t getClientCount() const;
};

#endif // MUSIC_SERVER_H