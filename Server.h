#pragma once

#include <vector>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "Client.h"
//#include <sys/time.h>

#pragma comment(lib, "Ws2_32.lib")

class Server {
public:
    Server(int maxClients, const char* port);
    ~Server();
    void run();
    void acceptClient();
    bool handleClientRequest(Client* client);
    void sendToSpecificClient(std::string message, Client* client);
    void sendToAllClients(std::string message, Client* sender);
    void logMessage(std::string message);
    void sendUdpBroadcast();
private:
    int maxClients;
    std::vector<Client*> clients;
    fd_set master;
    fd_set read_fds;
    SOCKET tcpServerSocket;
    SOCKET udpServerSocket;
    int fdmax;
    const char* port;
    std::string logFileName;
    int logFile;
    void initialize();
    timeval timeout;
    //Serer information
    std::string serverIP;
};
