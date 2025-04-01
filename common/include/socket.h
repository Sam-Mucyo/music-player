#ifndef SOCKET_H
#define SOCKET_H

#include <string>
#include <vector>
#include <cstring>
#include <stdexcept>
#include <iostream>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>

// Socket wrapper class for TCP communication
class Socket {
private:
    int sockfd;
    bool isConnected;

public:
    Socket() : sockfd(-1), isConnected(false) {}
    
    ~Socket() {
        close();
    }
    
    void close() {
        if (sockfd != -1) {
            ::close(sockfd);
            sockfd = -1;
            isConnected = false;
        }
    }
    
    // Create a server socket and bind to port
    bool createServer(int port) {
        // Create socket
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            std::cerr << "Error creating socket: " << strerror(errno) << std::endl;
            return false;
        }
        
        // Set socket option to reuse address
        int opt = 1;
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
            std::cerr << "Error setting socket options: " << strerror(errno) << std::endl;
            close();
            return false;
        }
        
        // Bind to port
        struct sockaddr_in serverAddr;
        memset(&serverAddr, 0, sizeof(serverAddr));
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = INADDR_ANY;
        serverAddr.sin_port = htons(port);
        
        if (bind(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
            std::cerr << "Error binding to port " << port << ": " << strerror(errno) << std::endl;
            close();
            return false;
        }
        
        // Start listening
        if (listen(sockfd, 5) < 0) {
            std::cerr << "Error listening on socket: " << strerror(errno) << std::endl;
            close();
            return false;
        }
        
        return true;
    }
    
    // Accept a client connection
    Socket* acceptClient() {
        struct sockaddr_in clientAddr;
        socklen_t clientAddrLen = sizeof(clientAddr);
        
        int clientSock = accept(sockfd, (struct sockaddr*)&clientAddr, &clientAddrLen);
        if (clientSock < 0) {
            std::cerr << "Error accepting connection: " << strerror(errno) << std::endl;
            return nullptr;
        }
        
        // Create a new Socket object for the client
        Socket* clientSocket = new Socket();
        clientSocket->sockfd = clientSock;
        clientSocket->isConnected = true;
        
        std::cout << "Client connected from " 
                  << inet_ntoa(clientAddr.sin_addr) << ":" 
                  << ntohs(clientAddr.sin_port) << std::endl;
        
        return clientSocket;
    }
    
    // Connect to a server
    bool connectToServer(const std::string& host, int port) {
        // Create socket
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            std::cerr << "Error creating socket: " << strerror(errno) << std::endl;
            return false;
        }
        
        // Resolve hostname
        struct hostent* server = gethostbyname(host.c_str());
        if (server == nullptr) {
            std::cerr << "Error resolving hostname " << host << ": " << strerror(errno) << std::endl;
            close();
            return false;
        }
        
        // Set up server address
        struct sockaddr_in serverAddr;
        memset(&serverAddr, 0, sizeof(serverAddr));
        serverAddr.sin_family = AF_INET;
        memcpy(&serverAddr.sin_addr.s_addr, server->h_addr, server->h_length);
        serverAddr.sin_port = htons(port);
        
        // Connect to server
        if (connect(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
            std::cerr << "Error connecting to " << host << ":" << port << ": " << strerror(errno) << std::endl;
            close();
            return false;
        }
        
        isConnected = true;
        return true;
    }
    
    // Send data over the socket
    bool send(const std::vector<char>& data) {
        if (!isConnected) {
            std::cerr << "Error: Socket not connected" << std::endl;
            return false;
        }
        
        ssize_t bytesSent = 0;
        ssize_t totalBytesSent = 0;
        ssize_t bytesRemaining = data.size();
        
        // Send all the data
        while (totalBytesSent < static_cast<ssize_t>(data.size())) {
            bytesSent = ::send(sockfd, data.data() + totalBytesSent, bytesRemaining, 0);
            
            if (bytesSent < 0) {
                std::cerr << "Error sending data: " << strerror(errno) << std::endl;
                return false;
            }
            
            totalBytesSent += bytesSent;
            bytesRemaining -= bytesSent;
        }
        
        return true;
    }
    
    // Receive data from the socket
    std::vector<char> receive(size_t length) {
        if (!isConnected) {
            std::cerr << "Error: Socket not connected" << std::endl;
            return std::vector<char>();
        }
        
        std::vector<char> buffer(length);
        ssize_t bytesRead = 0;
        ssize_t totalBytesRead = 0;
        ssize_t bytesRemaining = length;
        
        // Read all the requested data
        while (totalBytesRead < static_cast<ssize_t>(length)) {
            bytesRead = recv(sockfd, buffer.data() + totalBytesRead, bytesRemaining, 0);
            
            if (bytesRead <= 0) {
                // Connection closed or error
                if (bytesRead == 0) {
                    // Connection closed gracefully
                    isConnected = false;
                } else {
                    std::cerr << "Error receiving data: " << strerror(errno) << std::endl;
                }
                
                // Return what we've read so far
                buffer.resize(totalBytesRead);
                return buffer;
            }
            
            totalBytesRead += bytesRead;
            bytesRemaining -= bytesRead;
        }
        
        return buffer;
    }
    
    // Check if the socket is connected
    bool connected() const {
        return isConnected;
    }
    
    // Get the socket file descriptor
    int getSocketFd() const {
        return sockfd;
    }
};

#endif // SOCKET_H