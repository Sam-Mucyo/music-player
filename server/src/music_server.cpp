#include "music_server.h"
#include <iostream>

MusicServer::MusicServer(int serverPort, const std::string& musicDirectory)
    : port(serverPort), 
      musicDir(musicDirectory),
      serverSocket(new Socket()),
      isRunning(false) {
}

MusicServer::~MusicServer() {
    stop();
}

bool MusicServer::start() {
    // Create the music library
    library = std::make_shared<MusicLibrary>(musicDir);
    
    // Create and bind the server socket
    if (!serverSocket->createServer(port)) {
        std::cerr << "Failed to start server on port " << port << std::endl;
        return false;
    }
    
    std::cout << "Server started on port " << port << std::endl;
    std::cout << "Music directory: " << musicDir << std::endl;
    
    // Start the accept thread
    isRunning.store(true);
    acceptThread = std::thread(&MusicServer::acceptClients, this);
    
    return true;
}

void MusicServer::stop() {
    if (isRunning.load()) {
        isRunning.store(false);
        
        if (acceptThread.joinable()) {
            acceptThread.join();
        }
        
        // Stop all client handlers
        for (auto& client : clients) {
            client->stop();
        }
        clients.clear();
        
        serverSocket->close();
        
        std::cout << "Server stopped" << std::endl;
    }
}

bool MusicServer::running() const {
    return isRunning.load();
}

size_t MusicServer::getClientCount() const {
    return clients.size();
}

void MusicServer::acceptClients() {
    while (isRunning.load()) {
        // Clean up disconnected clients
        cleanupClients();
        
        // Accept a new client
        std::unique_ptr<Socket> clientSocket(serverSocket->acceptClient());
        
        if (clientSocket && clientSocket->connected()) {
            // Create a new client handler
            auto handler = std::make_unique<ClientHandler>(std::move(clientSocket), library);
            
            // Start handling the client
            handler->start();
            
            // Add to our list of clients
            clients.push_back(std::move(handler));
            
            std::cout << "New client connected. Total clients: " << clients.size() << std::endl;
        } else if (!isRunning.load()) {
            // Server is shutting down
            break;
        } else {
            // Accept failed, wait a bit and try again
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    
    std::cout << "Accept thread terminated" << std::endl;
}

void MusicServer::cleanupClients() {
    auto it = clients.begin();
    while (it != clients.end()) {
        if (!(*it)->isActive()) {
            it = clients.erase(it);
        } else {
            ++it;
        }
    }
}