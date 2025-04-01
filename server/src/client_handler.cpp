#include "client_handler.h"
#include <iostream>
#include "../../common/include/protocol.h"

// Size of audio chunks to send at once (256KB)
const size_t CHUNK_SIZE = 256 * 1024;

ClientHandler::ClientHandler(std::unique_ptr<Socket> socket, std::shared_ptr<MusicLibrary> musicLibrary)
    : clientSocket(std::move(socket)), 
      library(musicLibrary),
      isRunning(false) {
}

ClientHandler::~ClientHandler() {
    stop();
}

void ClientHandler::start() {
    isRunning.store(true);
    clientThread = std::thread(&ClientHandler::handleClient, this);
}

void ClientHandler::stop() {
    if (isRunning.load()) {
        isRunning.store(false);
        
        if (clientThread.joinable()) {
            clientThread.join();
        }
        
        clientSocket->close();
    }
}

bool ClientHandler::isActive() const {
    return isRunning.load();
}

void ClientHandler::handleClient() {
    while (isRunning.load() && clientSocket->connected()) {
        // Receive message header
        std::vector<char> headerData = clientSocket->receive(sizeof(MessageHeader));
        
        if (headerData.empty()) {
            if (clientSocket->connected()) {
                // Just no data yet, continue
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            } else {
                // Connection closed
                std::cout << "Client disconnected" << std::endl;
                break;
            }
        }
        
        if (headerData.size() < sizeof(MessageHeader)) {
            std::cerr << "Received incomplete header, closing connection" << std::endl;
            break;
        }
        
        // Parse the header
        MessageHeader header;
        memcpy(&header, headerData.data(), sizeof(MessageHeader));
        
        // Receive the message payload if there is one
        std::vector<char> payload;
        if (header.size > 0) {
            payload = clientSocket->receive(header.size);
            
            if (payload.size() < header.size) {
                std::cerr << "Received incomplete payload, closing connection" << std::endl;
                break;
            }
        }
        
        // Handle the message based on its type
        switch (header.type) {
            case MessageType::LIST_REQUEST:
                sendSongList();
                break;
                
            case MessageType::SONG_REQUEST:
                {
                    // Extract song name from payload
                    std::string songName(payload.begin(), payload.end());
                    std::cout << "Client requested song: " << songName << std::endl;
                    sendSong(songName);
                }
                break;
                
            case MessageType::PLAY_CONTROL:
                // Client-side controls don't require server action in this implementation
                break;
                
            default:
                std::cerr << "Received unknown message type: " << static_cast<int>(header.type) << std::endl;
                break;
        }
    }
    
    // Clean up
    isRunning.store(false);
    clientSocket->close();
    std::cout << "Client handler thread terminated" << std::endl;
}

bool ClientHandler::sendSongList() {
    const auto& songs = library->getSongList();
    
    // Serialize and send the song list
    std::vector<char> message = serializeMessage(MessageType::LIST_RESPONSE, songs);
    bool result = clientSocket->send(message);
    
    if (result) {
        std::cout << "Sent song list with " << songs.size() << " songs to client" << std::endl;
    } else {
        std::cerr << "Failed to send song list to client" << std::endl;
    }
    
    return result;
}

bool ClientHandler::sendSong(const std::string& songName) {
    // Check if the song exists
    if (!library->hasSong(songName)) {
        return sendError("Song not found: " + songName);
    }
    
    // Load the song
    auto song = library->getSong(songName);
    if (!song || !song->isLoaded()) {
        return sendError("Failed to load song: " + songName);
    }
    
    // Send the WAV header
    std::vector<char> headerMessage = serializeMessage(MessageType::SONG_INFO, song->getHeader());
    if (!clientSocket->send(headerMessage)) {
        std::cerr << "Failed to send song header" << std::endl;
        return false;
    }
    
    // Send the audio data in chunks
    const auto& audioData = song->getAudioData();
    size_t offset = 0;
    
    while (offset < audioData.size()) {
        // Serialize and send a chunk of audio data
        std::vector<char> dataMessage = serializeAudioData(audioData, offset, CHUNK_SIZE);
        
        if (!clientSocket->send(dataMessage)) {
            std::cerr << "Failed to send audio data chunk" << std::endl;
            return false;
        }
        
        // Update offset
        size_t chunkSize = std::min(CHUNK_SIZE, audioData.size() - offset);
        offset += chunkSize;
        
        // Small sleep to prevent overwhelming the network
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    // Send end marker
    std::vector<char> endMessage = serializeMessage(MessageType::SONG_DATA_END, std::string());
    bool result = clientSocket->send(endMessage);
    
    if (result) {
        std::cout << "Sent complete song: " << songName << " (" 
                  << audioData.size() << " bytes)" << std::endl;
    } else {
        std::cerr << "Failed to send song end marker" << std::endl;
    }
    
    return result;
}

bool ClientHandler::sendError(const std::string& errorMessage) {
    std::vector<char> message = serializeMessage(MessageType::ERROR, errorMessage);
    bool result = clientSocket->send(message);
    
    if (result) {
        std::cout << "Sent error to client: " << errorMessage << std::endl;
    } else {
        std::cerr << "Failed to send error message to client" << std::endl;
    }
    
    return result;
}