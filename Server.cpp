#include "Server.h"
#include <iostream>
#include "OutputValues.h"
#include <fstream>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#pragma warning(disable: 4996)
#define _CRT_SECURE_NO_WARNINGS

Server::Server(int maxClients, const char* port) : maxClients(maxClients), port(port), logFileName("chat_log.txt") {
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != NO_ERROR) 
    {
        std::cerr << "Error initializing Winsock2: " << result << std::endl;
        exit(STARTUP_ERROR);
    }
    initialize();

    struct addrinfo* result_addr = NULL, hints;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    result = getaddrinfo(NULL, port, &hints, &result_addr);
    if (result != 0) 
    {
        std::cerr << "Error getting address info: " << result << std::endl;
        WSACleanup();
        exit(ADDRESS_ERROR);
    }
    
    serverSocket = socket(result_addr->ai_family, result_addr->ai_socktype, result_addr->ai_protocol);
    if (serverSocket == INVALID_SOCKET) 
    {
        std::cerr << "Error creating server socket: " << WSAGetLastError() << std::endl;
        freeaddrinfo(result_addr);
        WSACleanup();
        exit(SETUP_ERROR);
    }

    result = bind(serverSocket, result_addr->ai_addr, (int)result_addr->ai_addrlen);
    if (result == SOCKET_ERROR) {
        std::cerr << "Error binding server socket: " << WSAGetLastError() << std::endl;
        freeaddrinfo(result_addr);
        closesocket(serverSocket);
        WSACleanup();
        exit(BIND_ERROR);
    }

    freeaddrinfo(result_addr);

    result = listen(serverSocket, SOMAXCONN);
    if (result == SOCKET_ERROR) 
    {
        std::cerr << "Error setting server socket to listen: " << WSAGetLastError() << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        exit(SETUP_ERROR);
    }
}

Server::~Server() {
    for (auto client : clients) {
        closesocket(client->getSocket());
        delete client;
    }
    closesocket(serverSocket);
    WSACleanup();
}


void Server::run() {
    // Prompt user for server IP and port
    std::string serverIP;
    std::cout << "Enter server IP address: ";
    std::cin >> serverIP;

    // Initialize server and start listening
    std::cout << "Starting server..." << std::endl;
    std::cout << "IP: " << serverIP << ", Port: " << port << std::endl;

    // Set up server socket to listen for incoming connections
    fdmax = serverSocket;
    FD_ZERO(&master);
    FD_ZERO(&read_fds);
    FD_SET(serverSocket, &master);
    timeout.tv_sec = 1;

    // Main loop for server
    while (true) 
    {
        read_fds = master;
        int highest_fd = serverSocket;
        for (const auto& client : clients) {
            if (client->getSocket() > highest_fd) {
                highest_fd = client->getSocket();
            }
        }
        int result = select(highest_fd + 1, &read_fds, NULL, NULL, &timeout);

        // Check for errors
        if (result == SOCKET_ERROR) 
        {
            std::cerr << "Error in select: " << WSAGetLastError() << std::endl;
            closesocket(serverSocket);
            WSACleanup();
            exit(PARAMETER_ERROR);
        }

        // Check if new client connection is available
        if (FD_ISSET(serverSocket, &read_fds)) 
        {
            acceptClient();
        }
        // Check all connected clients for incoming messages
        for (int i = 0; i < clients.size(); i++)
        {
            SOCKET clientSocket = clients[i]->getSocket();
            if (FD_ISSET(clientSocket, &read_fds))
            {
                handleClientRequest(clients[i]);
            }
        }

        // Remove clients with an INVALID_SOCKET
        for (int i = 0; i < clients.size();)
        {
            if (clients[i]->getSocket() == INVALID_SOCKET)
            {
                delete clients[i];
                clients.erase(clients.begin() + i);
            }
            else
            {
                ++i;
            }
        }
        //// Check all connected clients for incoming messages
        //for (int i = 0; i < clients.size(); i++)
        //{
        //    SOCKET clientSocket = clients[i]->getSocket();
        //    if (FD_ISSET(clientSocket, &read_fds))
        //    {
        //        handleClientRequest(clients[i]);
        //    }
        //    if (clients[i]->getSocket() == INVALID_SOCKET)
        //    {
        //        delete clients[i];
        //        clients.erase(clients.begin() + i);
        //    }
        //}
        //int clientSize = clients.size();
        //for (int i = 0; i < clientSize; i++) 
        //{
        //    SOCKET clientSocket = clients[i]->getSocket();
        //    // Check if the client socket is valid before proceeding
        //    if (clientSocket != INVALID_SOCKET && FD_ISSET(clientSocket, &read_fds))
        //    {
        //        handleClientRequest(clients[i]);
        //    }
        //    // After handling the request, check again if the socket is invalid
        //    // and remove the client if necessary
        //    if (clients[i]->getSocket() == INVALID_SOCKET)
        //    {
        //        delete clients[i];
        //        clients.erase(clients.begin() + i);
        //
        //        // Decrement the index to account for the removed client
        //        i--; clientSize--;
        //    }
        //}
    }
}

void Server::acceptClient() {
    struct sockaddr_in clientAddr {};
    int clientAddrLen = sizeof(clientAddr);

    SOCKET clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientAddrLen);

    // Check for errors
    if (clientSocket == INVALID_SOCKET) 
    {
        std::cerr << "Error accepting client socket: " << WSAGetLastError() << std::endl;
        return;
    }
    // Check if server is full
    if (clients.size() == maxClients) 
    {
        std::string message = "SV_FULL";
        send(clientSocket, message.c_str(), message.size() + 1, 0);
        closesocket(clientSocket);
        return;
    }
    // Add client to list of connected clients
    Client* newClient = new Client();
    newClient->setSocket(clientSocket);
    clients.push_back(newClient);
    // Add client socket to master set
    FD_SET(clientSocket, &master);
    if (clientSocket > fdmax) 
    {
        fdmax = clientSocket;
    }
    // Send success message to client
    std::string message = "SV_SUCCESS";
    send(clientSocket, message.c_str(), message.size() + 1, 0);
    // Print connection info
    std::cout << "New client connected from " << inet_ntoa(clientAddr.sin_addr) << ":" << ntohs(clientAddr.sin_port) << std::endl;
}

bool Server::handleClientRequest(Client* client) 
{
    // Receive the size of the incoming message first.
    int messageSize = 0;
    int nbytes = recv(client->getSocket(), reinterpret_cast<char*>(&messageSize), sizeof(messageSize), 0);
    if (nbytes <= 0)
    {
        // client disconnected
        closesocket(client->getSocket());
        FD_CLR(client->getSocket(), &master);
        for (auto it = clients.begin(); it != clients.end(); ++it)
        {
            if ((*it)->getSocket() == client->getSocket())
            {
                delete (*it);
                clients.erase(it);
                break;
            }
        }
        return false;
    }

    // Allocate a buffer of the appropriate size to receive the message.
    char* buffer = new char[messageSize];
    int bytesRead = 0;
    while (bytesRead < messageSize)
    {
        int result = recv(client->getSocket(), buffer + bytesRead, messageSize - bytesRead, 0);
        if (result == SOCKET_ERROR || result == 0)
        {
            // Error occurred or client disconnected
            delete[] buffer;
            closesocket(client->getSocket());
            FD_CLR(client->getSocket(), &master);
            for (auto it = clients.begin(); it != clients.end(); ++it)
            {
                if ((*it)->getSocket() == client->getSocket())
                {
                    delete (*it);
                    clients.erase(it);
                    break;
                }
            }
            return false;
        }
        bytesRead += result;
    }
    // Convert the received message to a string.
    std::string message(buffer, messageSize);
    std::cout<<"[Received] ("<< client->getUsername()<<"): " << message << std::endl;
    delete[] buffer;

    // Handle client request commands
    if (message.find("$register") == 0)
    {
        std::string username = message.substr(10, message.size() - 10);
        if (clients.size() >= maxClients)
        {
            // capacity full
            std::string reply = "SV_FULL";
            send(client->getSocket(), reply.c_str(), reply.size(), 0);
            closesocket(client->getSocket());
            FD_CLR(client->getSocket(), &master);
            return false;
        }
        else 
        {
            // register user
            client->setUsername(username);
            std::string reply = "SV_SUCCESS";
            send(client->getSocket(), reply.c_str(), reply.size(), 0);
        }
    }
    else if (message.find("$getlist") == 0)
    {
        // send list of connected users
        std::string list;
        list += "LIST ";
        for (auto& c : clients) 
        {
            list += c->getUsername() + ",";
        }
        if (clients.size() > 1)
        {
            list.erase(list.size() - 1); // remove last comma
        }
        else
        {
            list = "LIST You are all alone in this server\n";
        }
        sendToSpecificClient(list, client);
    }
    else if (message.find("$getlog") == 0)
    {
        // send chat log
        std::ifstream logFile(logFileName, std::ios::binary);
        if (!logFile.good()) {
            std::cerr << "Error opening log file." << std::endl;
            return false;
        }
        logFile.seekg(0, std::ios::end);
        int fileSize = logFile.tellg();
        logFile.seekg(0, std::ios::beg);

        std::string logPrefix = "LOG ";
        int totalSize = fileSize + logPrefix.size();

        char* fileBuffer = new char[totalSize];
        memcpy(fileBuffer, logPrefix.c_str(), logPrefix.size());
        logFile.read(fileBuffer + logPrefix.size(), fileSize);
        logFile.close();

        // Send the size of the log file with prefix first
        std::string logString(fileBuffer, totalSize);
        sendToSpecificClient(logString, client);
        delete[] fileBuffer;
    }
    else if (message.find("$exit") == 0) 
    { 
        // Send a message to the client before closing the connection
        std::string goodbyeMessage = "EXIT Goodbye! You have been disconnected.";
        //send(client->getSocket(), goodbyeMessage.c_str(), goodbyeMessage.size(), 0);
        sendToSpecificClient(goodbyeMessage, client);

        // Disable sending on the socket to give the client a chance to read the message
        shutdown(client->getSocket(), SD_SEND);

        // Wait for the client to acknowledge the message
        char buffer[256];
        recv(client->getSocket(), buffer, sizeof(buffer), 0);

        // user wants to exit
        closesocket(client->getSocket());
        FD_CLR(client->getSocket(), &master);
        for (auto it = clients.begin(); it != clients.end(); ++it)
        {
            if ((*it)->getSocket() == client->getSocket())
            {
                delete (*it);
                clients.erase(it);
                break;
            }
        }
        //// Send a message to the client before closing the connection
        //std::string goodbyeMessage = "Goodbye! You have been disconnected.";
        //send(client->getSocket(), goodbyeMessage.c_str(), goodbyeMessage.size(), 0);
        //// user wants to exit
        //closesocket(client->getSocket());
        //FD_CLR(client->getSocket(), &master);
        //for (auto it = clients.begin(); it != clients.end(); ++it)
        //{
        //    if ((*it)->getSocket() == client->getSocket())
        //    {
        //        delete (*it);
        //        clients.erase(it);
        //        break;
        //    }
        //}
    }
    else if (message.find("$chat") == 0)
    {
        // broadcast message to all other clients
        std::string broadcastMessage = "(" + client->getUsername() + "): " + message.substr(6);
        broadcastMessage = "\nCHAT " + broadcastMessage;
        sendToAllClients(broadcastMessage, client);
        logMessage(broadcastMessage);
    }
    else 
    {
        // broadcast message to all other clients
        std::string broadcastMessage = "(" + client->getUsername() + "): " + message;
        broadcastMessage = "CHAT " + broadcastMessage;
        sendToAllClients(broadcastMessage, client);
        logMessage(broadcastMessage);
    }
    return true;
}

void Server::sendToSpecificClient(std::string message, Client* client) {
    // Send the size of the message first
    //uint32_t messageSize = htonl(static_cast<uint32_t>(message.size()));
    uint32_t messageSize = static_cast<uint32_t>(message.size());
    send(client->getSocket(), (const char*)&messageSize, sizeof(messageSize), 0);

    // Send the actual message
    send(client->getSocket(), message.c_str(), message.size(), 0);
}

void Server::sendToAllClients(std::string message, Client* sender) {
    // Send message size to all clients except sender
    int messageSize = static_cast<int>(message.length());
    for (auto& client : clients)
    {
        if (client->getSocket() != serverSocket && client->getSocket() != sender->getSocket())
        {
            int result = send(client->getSocket(), reinterpret_cast<char*>(&messageSize), sizeof(messageSize), 0);
            if (result == SOCKET_ERROR)
            {
                throw std::runtime_error("Failed to send message size: " + std::to_string(WSAGetLastError()));
            }

            // Send message to client
            result = send(client->getSocket(), message.c_str(), messageSize, 0);
            if (result == SOCKET_ERROR)
            {
                throw std::runtime_error("Failed to send message: " + std::to_string(WSAGetLastError()));
            }
        }
    }
}

void Server::logMessage(std::string message) {
    std::ofstream logFile(logFileName, std::ios::app);
    if (!logFile.good()) {
        std::cerr << "Error opening log file." << std::endl;
        return;
    }
    std::time_t now = std::time(nullptr);
    std::tm* localTime = std::localtime(&now);
    char timeBuffer[80];
    std::strftime(timeBuffer, sizeof(timeBuffer), "[%Y-%m-%d %H:%M:%S]", localTime);
    logFile << timeBuffer << " " << message << std::endl;
    logFile.close();
}

void Server::initialize() {
    // clear the fd sets
    FD_ZERO(&master);
    FD_ZERO(&read_fds);

    // add the server socket to the master set
    FD_SET(serverSocket, &master);

    // set the timeout value for select
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    // set the initial maximum file descriptor to the server socket
    fdmax = serverSocket;
}


