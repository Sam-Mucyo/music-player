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

/**
 * @file music_client.h
 * @brief Client implementation for the Network Music Player
 * @author CS262 Project Team
 */

/**
 * @class MusicClient
 * @brief Main client class that communicates with the music server
 *
 * This class handles the network connection to the server, manages message
 * passing according to the protocol, and controls audio playback through
 * the AudioPlayer class.
 */
class MusicClient {
private:
    std::unique_ptr<Socket> socket;         ///< Network socket for server communication
    std::unique_ptr<AudioPlayer> player;    ///< Audio playback component
    std::atomic<bool> isRunning;            ///< Flag indicating if client is running
    std::thread receiveThread;              ///< Thread for handling incoming messages
    std::string currentSong;                ///< Name of the currently loaded song
    std::vector<std::string> availableSongs; ///< List of songs available on the server
    
    /// Buffer for receiving audio data from the server
    std::vector<char> audioBuffer;
    
    /// Flag indicating if client is currently buffering audio data
    bool isBuffering;
    
    /**
     * @brief Processes received messages from the server
     * @param header The message header containing type and size information
     * @param data The message payload data
     */
    void handleMessage(const MessageHeader& header, const std::vector<char>& data);
    
    /**
     * @brief Thread function that continuously receives data from the server
     * 
     * This function runs in a separate thread and handles incoming messages
     * from the server, passing them to handleMessage() for processing.
     */
    void receiveThreadFunc();
    
    /**
     * @brief Parses a byte array into a list of strings
     * @param data The raw data containing the string list
     * @return A vector of strings parsed from the data
     */
    std::vector<std::string> parseStringList(const std::vector<char>& data);

public:
    /**
     * @brief Constructs a new Music Client object
     */
    MusicClient();
    
    /**
     * @brief Destroys the Music Client object
     * 
     * Ensures proper cleanup of resources, including stopping playback
     * and disconnecting from the server.
     */
    ~MusicClient();
    
    /**
     * @brief Connect to a music server
     * @param host The hostname or IP address of the server
     * @param port The port number to connect to
     * @return true if connection successful, false otherwise
     */
    bool connect(const std::string& host, int port);
    
    /**
     * @brief Disconnect from the server
     * 
     * Closes the socket connection and stops any ongoing operations.
     */
    void disconnect();
    
    /**
     * @brief Request the list of available songs from the server
     * @return true if request was sent successfully, false otherwise
     */
    bool requestSongList();
    
    /**
     * @brief Request a specific song from the server
     * @param songName The name of the song to request
     * @return true if request was sent successfully, false otherwise
     */
    bool requestSong(const std::string& songName);
    
    /**
     * @brief Start or resume playback of the current song
     * @return true if successful, false otherwise
     */
    bool play();
    
    /**
     * @brief Pause playback of the current song
     * @return true if successful, false otherwise
     */
    bool pause();
    
    /**
     * @brief Stop playback of the current song
     * @return true if successful, false otherwise
     */
    bool stop();
    
    /**
     * @brief Seek to a specific position in the current song
     * @param position The position in seconds to seek to
     * @return true if successful, false otherwise
     */
    bool seek(double position);
    
    /**
     * @brief Get the current playback position
     * @return The current position in seconds
     */
    double getCurrentPosition() const;
    
    /**
     * @brief Get the total duration of the current song
     * @return The duration in seconds
     */
    double getDuration() const;
    
    /**
     * @brief Check if a song is currently playing
     * @return true if playing, false if paused or stopped
     */
    bool isPlaying() const;
    
    /**
     * @brief Get the name of the currently loaded song
     * @return The name of the current song
     */
    std::string getCurrentSong() const;
    
    /**
     * @brief Get the list of available songs from the server
     * @return A vector of song names
     */
    const std::vector<std::string>& getAvailableSongs() const;
    
    /**
     * @brief Check if the client is connected to a server
     * @return true if connected, false otherwise
     */
    bool isConnected() const;
};

#endif // MUSIC_CLIENT_H