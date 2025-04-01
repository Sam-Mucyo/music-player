#include <iostream>
#include <string>
#include <csignal>
#include "music_server.h"

// Global server instance for signal handling
MusicServer* g_server = nullptr;

// Signal handler for graceful shutdown
void signalHandler(int signum) {
    std::cout << "\nReceived signal " << signum << ", shutting down..." << std::endl;
    
    if (g_server) {
        g_server->stop();
    }
    
    exit(signum);
}

int main(int argc, char* argv[]) {
    int port = 8080;
    std::string musicDir = "./music";
    
    // Parse command line arguments
    if (argc >= 2) {
        try {
            port = std::stoi(argv[1]);
        } catch (const std::exception& e) {
            std::cerr << "Invalid port number: " << argv[1] << std::endl;
            std::cerr << "Usage: " << argv[0] << " [port] [music_directory]" << std::endl;
            return 1;
        }
    }
    
    if (argc >= 3) {
        musicDir = argv[2];
    }
    
    // Register signal handlers
    signal(SIGINT, signalHandler);  // Ctrl+C
    signal(SIGTERM, signalHandler); // Termination signal
    
    std::cout << "Music Player Server" << std::endl;
    std::cout << "Starting server on port " << port << " with music directory: " << musicDir << std::endl;
    
    // Create and start the server
    MusicServer server(port, musicDir);
    g_server = &server;
    
    if (!server.start()) {
        std::cerr << "Failed to start server" << std::endl;
        return 1;
    }
    
    std::cout << "Server is running. Press Ctrl+C to stop." << std::endl;
    
    // Main loop
    std::string command;
    while (server.running()) {
        std::cout << "> ";
        std::getline(std::cin, command);
        
        if (command == "clients") {
            std::cout << "Connected clients: " << server.getClientCount() << std::endl;
        } else if (command == "stop" || command == "exit" || command == "quit") {
            std::cout << "Stopping server..." << std::endl;
            server.stop();
            break;
        } else if (command == "help") {
            std::cout << "Commands:" << std::endl;
            std::cout << "  clients    - Show number of connected clients" << std::endl;
            std::cout << "  stop/exit  - Stop the server" << std::endl;
            std::cout << "  help       - Show this help" << std::endl;
        } else if (!command.empty()) {
            std::cout << "Unknown command. Type 'help' for available commands." << std::endl;
        }
    }
    
    std::cout << "Server stopped." << std::endl;
    g_server = nullptr;
    
    return 0;
}