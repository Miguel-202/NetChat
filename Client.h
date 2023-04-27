#pragma once

#include <string>
#include <winsock2.h>

#pragma comment(lib, "Ws2_32.lib")

class Client {
public:
    Client();
    ~Client();
    void connectToServer(const char* serverIP, const char* port);
    void registerUser(std::string username);
    void executeCommand(std::string command);
    void sendMessage(std::string message);
    void closeConnection();
    std::string receiveMessage(bool& flag);
    bool isConnected();
    void setSocket(SOCKET newSocket);
    SOCKET getSocket() const;
    std::string getUsername() const;
    void setUsername(std::string newUsername);
    void listenForUdpBroadcast();
private:
    SOCKET clientSocket;
    SOCKET udpClientSocket;
    bool connected;
    std::string username;

    std::string logFileName;
};



