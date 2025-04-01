#include "music_client.h"
#include <chrono>
#include <iostream>

MusicClient::MusicClient() 
    : socket(new Socket()), 
      player(new AudioPlayer()), 
      isRunning(false),
      isBuffering(false) {
}

MusicClient::~MusicClient() {
    disconnect();
}

bool MusicClient::connect(const std::string& host, int port) {
    if (!socket->connectToServer(host, port)) {
        return false;
    }
    
    // Start receive thread
    isRunning.store(true);
    receiveThread = std::thread(&MusicClient::receiveThreadFunc, this);
    
    // Request song list
    requestSongList();
    
    return true;
}

void MusicClient::disconnect() {
    if (isRunning.load()) {
        isRunning.store(false);
        
        if (receiveThread.joinable()) {
            receiveThread.join();
        }
        
        socket->close();
        
        // Stop any playing audio
        if (player) {
            player->stop();
        }
    }
}

bool MusicClient::requestSongList() {
    // Create empty message to request song list
    std::vector<char> message = serializeMessage(MessageType::LIST_REQUEST, std::string());
    return socket->send(message);
}

bool MusicClient::requestSong(const std::string& songName) {
    currentSong = songName;
    // Clear any existing audio data
    player->clearAudioData();
    isBuffering = true;
    
    // Send song request
    std::vector<char> message = serializeMessage(MessageType::SONG_REQUEST, songName);
    return socket->send(message);
}

void MusicClient::receiveThreadFunc() {
    while (isRunning.load()) {
        // First, receive the message header
        std::vector<char> headerData = socket->receive(sizeof(MessageHeader));
        
        if (headerData.empty()) {
            if (socket->connected()) {
                // Just no data yet, continue
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            } else {
                // Connection closed
                std::cerr << "Connection closed by server" << std::endl;
                break;
            }
        }
        
        if (headerData.size() < sizeof(MessageHeader)) {
            std::cerr << "Received incomplete header" << std::endl;
            continue;
        }
        
        // Parse the header
        MessageHeader header;
        memcpy(&header, headerData.data(), sizeof(MessageHeader));
        
        // Receive the message payload
        std::vector<char> payload = socket->receive(header.size);
        
        if (payload.size() < header.size) {
            std::cerr << "Received incomplete payload" << std::endl;
            continue;
        }
        
        // Handle the message
        handleMessage(header, payload);
    }
}

void MusicClient::handleMessage(const MessageHeader& header, const std::vector<char>& data) {
    switch (header.type) {
        case MessageType::LIST_RESPONSE:
            availableSongs = parseStringList(data);
            std::cout << "Received song list with " << availableSongs.size() << " songs:" << std::endl;
            for (size_t i = 0; i < availableSongs.size(); ++i) {
                std::cout << (i + 1) << ". " << availableSongs[i] << std::endl;
            }
            break;
            
        case MessageType::SONG_INFO:
            if (data.size() >= sizeof(WavHeader)) {
                WavHeader header;
                memcpy(&header, data.data(), sizeof(WavHeader));
                
                // Initialize the audio player with this header
                player->initialize(header);
                audioBuffer.clear();
                std::cout << "Received song info, waiting for data..." << std::endl;
            }
            break;
            
        case MessageType::SONG_DATA:
            // Add the audio data to our buffer
            audioBuffer.insert(audioBuffer.end(), data.begin(), data.end());
            
            // If we're still buffering and have enough data, start playback
            if (isBuffering && audioBuffer.size() > 1024 * 1024) { // 1MB buffer
                player->addAudioData(audioBuffer);
                audioBuffer.clear();
                isBuffering = false;
                
                std::cout << "Starting playback of " << currentSong << std::endl;
                player->play();
            } else if (!isBuffering) {
                // If we're already playing, add this data to the player
                player->addAudioData(data);
            }
            break;
            
        case MessageType::SONG_DATA_END:
            // Add any remaining buffered data
            if (!audioBuffer.empty()) {
                player->addAudioData(audioBuffer);
                audioBuffer.clear();
            }
            
            if (isBuffering) {
                isBuffering = false;
                player->play();
            }
            
            std::cout << "Received complete song data for " << currentSong << std::endl;
            break;
            
        case MessageType::ERROR:
            {
                // Convert data to string for error message
                std::string errorMsg(data.begin(), data.end());
                std::cerr << "Error from server: " << errorMsg << std::endl;
                break;
            }
            
        default:
            std::cerr << "Received unknown message type: " << static_cast<int>(header.type) << std::endl;
            break;
    }
}

std::vector<std::string> MusicClient::parseStringList(const std::vector<char>& data) {
    std::vector<std::string> result;
    
    if (data.size() < 4) {
        return result; // Not enough data for count
    }
    
    // Extract count
    uint32_t count;
    memcpy(&count, data.data(), 4);
    
    size_t offset = 4;
    for (uint32_t i = 0; i < count; ++i) {
        // Check if we have enough data for the length
        if (offset + 4 > data.size()) {
            break;
        }
        
        // Extract string length
        uint32_t length;
        memcpy(&length, data.data() + offset, 4);
        offset += 4;
        
        // Check if we have enough data for the string
        if (offset + length > data.size()) {
            break;
        }
        
        // Extract string
        std::string str(data.data() + offset, data.data() + offset + length);
        result.push_back(str);
        offset += length;
    }
    
    return result;
}

bool MusicClient::play() {
    if (isBuffering) {
        std::cout << "Still buffering, please wait..." << std::endl;
        return false;
    }
    
    return player->play();
}

bool MusicClient::pause() {
    return player->pause();
}

bool MusicClient::stop() {
    return player->stop();
}

bool MusicClient::seek(double position) {
    return player->seekToPosition(position);
}

double MusicClient::getCurrentPosition() const {
    return player->getPositionInSeconds();
}

double MusicClient::getDuration() const {
    return player->getDurationInSeconds();
}

bool MusicClient::isPlaying() const {
    return player->isPlaying();
}

std::string MusicClient::getCurrentSong() const {
    return currentSong;
}

const std::vector<std::string>& MusicClient::getAvailableSongs() const {
    return availableSongs;
}

bool MusicClient::isConnected() const {
    return socket->connected();
}