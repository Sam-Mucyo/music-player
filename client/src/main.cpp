#include <iostream>
#include <string>
#include "music_client.h"

void displayHelp() {
    std::cout << "\nCommands:" << std::endl;
    std::cout << "  list              - Show available songs" << std::endl;
    std::cout << "  play <song_number>- Request and play a song by number" << std::endl;
    std::cout << "  resume            - Resume playback" << std::endl;
    std::cout << "  pause             - Pause playback" << std::endl;
    std::cout << "  stop              - Stop playback" << std::endl;
    std::cout << "  seek <seconds>    - Seek to position" << std::endl;
    std::cout << "  position          - Show current position" << std::endl;
    std::cout << "  duration          - Show song duration" << std::endl;
    std::cout << "  help              - Show this help" << std::endl;
    std::cout << "  exit              - Exit the client" << std::endl;
}

int main(int argc, char* argv[]) {
    std::string serverHost = "localhost";
    int serverPort = 8080;
    
    // Parse command line arguments
    if (argc >= 2) {
        serverHost = argv[1];
    }
    
    if (argc >= 3) {
        try {
            serverPort = std::stoi(argv[2]);
        } catch (const std::exception& e) {
            std::cerr << "Invalid port number: " << argv[2] << std::endl;
            std::cerr << "Usage: " << argv[0] << " [host] [port]" << std::endl;
            return 1;
        }
    }
    
    std::cout << "Music Player Client" << std::endl;
    std::cout << "Connecting to " << serverHost << ":" << serverPort << "..." << std::endl;
    
    MusicClient client;
    
    // Connect to server
    if (!client.connect(serverHost, serverPort)) {
        std::cerr << "Failed to connect to server" << std::endl;
        return 1;
    }
    
    std::cout << "Connected to server" << std::endl;
    displayHelp();
    
    // Main command loop
    std::string command;
    while (true) {
        std::cout << "> ";
        std::getline(std::cin, command);
        
        if (command == "list") {
            client.requestSongList();
            
        } else if (command.substr(0, 5) == "play ") {
            try {
                int songIndex = std::stoi(command.substr(5)) - 1;
                const auto& songs = client.getAvailableSongs();
                
                if (songIndex >= 0 && songIndex < static_cast<int>(songs.size())) {
                    client.requestSong(songs[songIndex]);
                } else {
                    std::cout << "Invalid song number. Use 'list' to see available songs." << std::endl;
                }
            } catch (const std::exception& e) {
                std::cout << "Invalid song number. Use 'list' to see available songs." << std::endl;
            }
            
        } else if (command == "resume") {
            client.play();
            
        } else if (command == "pause") {
            client.pause();
            
        } else if (command == "stop") {
            client.stop();
            
        } else if (command.substr(0, 5) == "seek ") {
            try {
                double seconds = std::stod(command.substr(5));
                client.seek(seconds);
            } catch (const std::exception& e) {
                std::cout << "Invalid position. Usage: seek <seconds>" << std::endl;
            }
            
        } else if (command == "position") {
            std::cout << "Current position: " << client.getCurrentPosition() 
                      << " seconds" << std::endl;
            
        } else if (command == "duration") {
            std::cout << "Song duration: " << client.getDuration() 
                      << " seconds" << std::endl;
            
        } else if (command == "help") {
            displayHelp();
            
        } else if (command == "exit") {
            break;
            
        } else {
            std::cout << "Unknown command. Type 'help' for available commands." << std::endl;
        }
        
        // Check if we're still connected
        if (!client.isConnected()) {
            std::cerr << "Lost connection to server" << std::endl;
            break;
        }
    }
    
    std::cout << "Disconnecting from server..." << std::endl;
    client.disconnect();
    
    return 0;
}