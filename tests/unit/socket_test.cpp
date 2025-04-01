#include <gtest/gtest.h>
#include "socket.h"
#include <thread>
#include <chrono>
#include <vector>

class SocketTest : public ::testing::Test {
protected:
    const int TEST_PORT = 8999;  // Use a port unlikely to be in use
    
    void SetUp() override {
        // Wait a moment to ensure any sockets from previous tests are closed
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    void TearDown() override {
        // Wait a moment to ensure any sockets are closed before next test
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
};

TEST_F(SocketTest, CreateServerSocket) {
    Socket serverSocket;
    EXPECT_TRUE(serverSocket.createServer(TEST_PORT));
    
    // Verify the socket is valid
    EXPECT_NE(serverSocket.getSocketFd(), -1);
    
    // Test socket is bound by trying to bind another socket to the same port
    Socket anotherSocket;
    EXPECT_FALSE(anotherSocket.createServer(TEST_PORT));
}

TEST_F(SocketTest, ConnectToServer) {
    // Start a server in a separate thread
    std::thread serverThread([this]() {
        Socket serverSocket;
        ASSERT_TRUE(serverSocket.createServer(TEST_PORT));
        
        Socket clientSocket;
        ASSERT_TRUE(serverSocket.acceptClient(clientSocket));
        
        // Send a test message
        std::vector<char> testMsg = {'H', 'e', 'l', 'l', 'o'};
        ASSERT_TRUE(clientSocket.send(testMsg.data(), testMsg.size()));
        
        // Close after communication
        serverSocket.close();
    });
    
    // Give the server a moment to start
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Connect client
    Socket clientSocket;
    EXPECT_TRUE(clientSocket.connectToServer("localhost", TEST_PORT));
    
    // Receive the test message
    std::vector<char> buffer(5);
    int bytesRead = clientSocket.receive(buffer.data(), buffer.size());
    EXPECT_EQ(bytesRead, 5);
    EXPECT_EQ(std::string(buffer.begin(), buffer.end()), "Hello");
    
    // Wait for server thread to finish
    serverThread.join();
}

TEST_F(SocketTest, NonBlockingReceive) {
    // Create a server socket
    Socket serverSocket;
    ASSERT_TRUE(serverSocket.createServer(TEST_PORT));
    
    // Connect in a separate thread
    std::thread clientThread([this]() {
        // Give the server a moment to start accepting
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        Socket clientSocket;
        ASSERT_TRUE(clientSocket.connectToServer("localhost", TEST_PORT));
        
        // Wait a moment before sending data
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        
        // Send a test message
        std::vector<char> testMsg = {'D', 'a', 't', 'a'};
        ASSERT_TRUE(clientSocket.send(testMsg.data(), testMsg.size()));
        
        clientSocket.close();
    });
    
    // Accept client connection
    Socket clientConnection;
    ASSERT_TRUE(serverSocket.acceptClient(clientConnection));
    
    // Try non-blocking receive before data arrives
    std::vector<char> buffer(10);
    int bytesRead = clientConnection.receiveNonBlocking(buffer.data(), buffer.size());
    EXPECT_EQ(bytesRead, 0);  // No data available yet
    
    // Wait for data to arrive
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    // Try non-blocking receive after data arrives
    bytesRead = clientConnection.receiveNonBlocking(buffer.data(), buffer.size());
    EXPECT_EQ(bytesRead, 4);
    EXPECT_EQ(std::string(buffer.begin(), buffer.begin() + bytesRead), "Data");
    
    // Wait for client thread to finish
    clientThread.join();
    serverSocket.close();
}

// Note: In a real test suite, you might want to add more tests for handling:
// - Connection failures
// - Timeouts
// - Large data transfers
// - Error conditions