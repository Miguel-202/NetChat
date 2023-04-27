#include "Client.h"

#include <string>
#include "OutputValues.h"
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <vector>

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#pragma warning(disable: 4996)

Client::Client() : logFileName("client_log.txt")
{
    // Initialize Winsock.
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) 
    {
        throw std::runtime_error("WSAStartup failed: " + std::to_string(result));
    }
    // Create a socket.
    clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket == INVALID_SOCKET) 
    {
        WSACleanup();
        throw std::runtime_error("Failed to create socket: " + std::to_string(WSAGetLastError()));
    }

    // Create a UDP socket.
    udpClientSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (udpClientSocket == INVALID_SOCKET) 
    {
        WSACleanup();
        throw std::runtime_error("Failed to create UDP socket: " + std::to_string(WSAGetLastError()));
    }

    // Set socket options for broadcast and reuse.
    int broadcastEnable = 1;
    result = setsockopt(udpClientSocket, SOL_SOCKET, SO_BROADCAST, (char*)&broadcastEnable, sizeof(broadcastEnable));
    if (result == SOCKET_ERROR) 
    {
        closesocket(udpClientSocket);
        WSACleanup();
        throw std::runtime_error("Failed to set UDP socket options: " + std::to_string(WSAGetLastError()));
    }

    connected = false;
}

Client::~Client() 
{
    if (connected) 
    {
        closeConnection();
    }
    // Close the UDP socket.
    closesocket(udpClientSocket);
    WSACleanup();
}

void Client::listenForUdpBroadcast()
{
    sockaddr_in udpClientAddr{};
    udpClientAddr.sin_family = AF_INET;
    udpClientAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    udpClientAddr.sin_port = htons(5000); //static_cast<unsigned short>(std::stoi(port)) in case of having a port number as const char* port similar to the server

    // Bind the UDP socket.
    int result = bind(udpClientSocket, (sockaddr*)&udpClientAddr, sizeof(udpClientAddr));
    if (result == SOCKET_ERROR) {
        throw std::runtime_error("Failed to bind UDP socket: " + std::to_string(WSAGetLastError()));
    }

    //// Set the UDP socket to non-blocking mode. Could fix the problem but It gives the WSAEWOULDBLOCK non blocking error 1003
    //u_long mode = 1;
    //if (ioctlsocket(udpClientSocket, FIONBIO, &mode) != 0) {
    //    throw std::runtime_error("Failed to set UDP socket to non-blocking mode: " + std::to_string(WSAGetLastError()));
    //}

    // Set the UDP socket to receive broadcast messages.
    int broadcastPermission = 1;
    if (setsockopt(udpClientSocket, SOL_SOCKET, SO_BROADCAST, (char*)&broadcastPermission, sizeof(broadcastPermission)) < 0) 
    {
        throw std::runtime_error("Failed to set SO_BROADCAST for UDP client socket: " + std::to_string(WSAGetLastError()));
    }

    // Receive server broadcast.
    char recvBuffer[256];
    sockaddr_in serverBroadcastAddr{};
    int serverBroadcastAddrSize = sizeof(serverBroadcastAddr);
    result = recvfrom(udpClientSocket, recvBuffer, sizeof(recvBuffer) - 1, 0, (sockaddr*)&serverBroadcastAddr, &serverBroadcastAddrSize);
    if (result == SOCKET_ERROR) 
    {
        throw std::runtime_error("Failed to receive UDP broadcast: " + std::to_string(WSAGetLastError()));
    }
    recvBuffer[result] = '\0';

    // Extract server IP and port from the received broadcast.
    std::string broadcastMessage(recvBuffer);
    size_t separatorPos = broadcastMessage.find(':');
    if (separatorPos == std::string::npos) {
        throw std::runtime_error("Invalid broadcast message received");
    }
    std::string receivedServerIP = broadcastMessage.substr(0, separatorPos);
    std::string receivedServerPort = broadcastMessage.substr(separatorPos + 1);

    // Connect to the server using the received IP and port.
    connectToServer(receivedServerIP.c_str(), receivedServerPort.c_str());
    closesocket(udpClientSocket);
}

void Client::connectToServer(const char* serverIP, const char* port) 
{
    // Create a sockaddr_in object and set its values.
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr(serverIP);
    serverAddress.sin_port = htons(atoi(port));
    // Call the connect function, passing the created socket and the sockaddr_in structure as parameters.
    int result = connect(clientSocket, (SOCKADDR*)&serverAddress, sizeof(serverAddress));
    if (result == SOCKET_ERROR) 
    {
        throw std::runtime_error("Failed to connect to server: " + std::to_string(WSAGetLastError()));
    }

    connected = true;
}

void Client::registerUser(std::string username)
{
    if (!connected)
    {
        throw std::runtime_error("Client is not connected to server");
    }

    // Save the provided username.
    this->username = username;

    // Create the register command string.
    std::string registerCommand = "$register " + username;

    // Send the size of the command first.
    int commandSize = static_cast<int>(registerCommand.length());
    int result = send(clientSocket, reinterpret_cast<char*>(&commandSize), sizeof(commandSize), 0);
    if (result == SOCKET_ERROR)
    {
        throw std::runtime_error("Failed to send command size: " + std::to_string(WSAGetLastError()));
    }

    // Send the register command to the server.
    result = send(clientSocket, registerCommand.c_str(), commandSize, 0);
    if (result == SOCKET_ERROR)
    {
        throw std::runtime_error("Failed to send register command: " + std::to_string(WSAGetLastError()));
    }

    // Receive server response.
    char responseBuffer[256];
    result = recv(clientSocket, responseBuffer, sizeof(responseBuffer), 0);
    if (result == SOCKET_ERROR)
    {
        throw std::runtime_error("Failed to receive server response: " + std::to_string(WSAGetLastError()));
    }

    // Check server response.
    std::string response(responseBuffer, result);
    std::string success = "SV_SUCCESS\0";
    std::string serverFullMessage = "SV_FULL\0";
    if (response.back() == '\n')
    {
        response.pop_back();
    }
    if (response == "SV_FULL")
    {
        closeConnection();
        throw std::runtime_error("Server is full. Please try again later.");
    }
    else if (response != success)
    {
        bool areEqual = true;
        bool serverFull = true;
        for (size_t i = 0; i < response.length(); i++)//loop to fix the server success message because the server sends it with garbage at the end
        {
            if (response[i] != success[i])
            {
                areEqual = false;
                break;
            }
        }
        for (size_t i = 0; i < response.length(); i++)//loop to fix the server full message because the server sends it with garbage at the end
        {
            if (response[i] != serverFullMessage[i])
            {
                serverFull = false;
                break;
            }
        }
        if (serverFull)
        {
			closeConnection();
			throw std::runtime_error("Server is full. Please try again later.");
		}
        else if (!areEqual)
        {
            throw std::runtime_error("Failed to register user: " + response);
        }
    }

}

void Client::executeCommand(std::string command) 
{
    if (!connected)
    {
        throw std::runtime_error("Client is not connected to server");
    }

    // Send command size to server first
    int commandSize = static_cast<int>(command.length());
    int result = send(clientSocket, reinterpret_cast<char*>(&commandSize), sizeof(commandSize), 0);
    if (result == SOCKET_ERROR)
    {
        throw std::runtime_error("Failed to send command size: " + std::to_string(WSAGetLastError()));
    }

    // Send command to server
    result = send(clientSocket, command.c_str(), commandSize, 0);
    if (result == SOCKET_ERROR)
    {
        throw std::runtime_error("Failed to send command: " + std::to_string(WSAGetLastError()));
    }
    std::cout << "[Executed] " << command << std::endl;
}

void Client::sendMessage(std::string message) 
{
    if (!connected)
    {
        throw std::runtime_error("Client is not connected to server");
    }

    std::string chatCommand = "$chat " + message;

    // Send message size to server first
    int messageSize = static_cast<int>(chatCommand.length());
    int result = send(clientSocket, reinterpret_cast<char*>(&messageSize), sizeof(messageSize), 0);
    if (result == SOCKET_ERROR)
    {
        throw std::runtime_error("Failed to send message size: " + std::to_string(WSAGetLastError()));
    }

    // Send message to server
    result = send(clientSocket, chatCommand.c_str(), messageSize, 0);
    if (result == SOCKET_ERROR)
    {
        throw std::runtime_error("Failed to send chat message: " + std::to_string(WSAGetLastError()));
    }
    std::cout<<"[Sent out] " << message << std::endl;
}

void Client::closeConnection() 
{
    if (!connected) 
    {
        return;
    }
    // Shutdown the connection.
    int result = shutdown(clientSocket, SD_SEND);
    if (result == SOCKET_ERROR) 
    {
        throw std::runtime_error("Failed to shutdown connection: " + std::to_string(WSAGetLastError()));
    }
    std::cout << "\nDisconnected\n";
    closesocket(clientSocket);
    connected = false;
}

std::string Client::receiveMessage(bool& flag)
{
    if (!connected)
    {
        throw std::runtime_error("Client is not connected to server");
    }

    while (true)
    {
        // Receive the size of the incoming message first.
        int messageSize = 0;
        int nbytes = recv(clientSocket, reinterpret_cast<char*>(&messageSize), sizeof(messageSize), 0);
        if (nbytes <= 0)
        {
            throw std::runtime_error("Failed to receive message size: " + std::to_string(WSAGetLastError()));
        }

        // Allocate a buffer of the appropriate size to receive the message.
        char* buffer = new char[messageSize];
        nbytes = recv(clientSocket, buffer, messageSize, 0);
        if (nbytes <= 0)
        {
            delete[] buffer;
            throw std::runtime_error("Failed to receive message: " + std::to_string(WSAGetLastError()));
        }

        // Convert the received message to a string and return it if valid.
        std::string message(buffer, messageSize);
        delete[] buffer;
        if (message.find("SV_SUCCESS") == 0 || message.find("SV_FULL") == 0)
        {
            return message;
        }
        else if (message.find("CHAT") == 0)
        {   // If the message is a chat message, print it to the console. And delete the current line
            return ("\033[2K\r" + message + "\nEnter command or message: ");
        }
        else if (message.find("LIST") == 0)
        {
            return ("\033[2K\r" + message.substr(5) + "\nEnter command or message: ");
        }
        else if (message.find("LOG") == 0)
        {
            //check if clientLog.txt exists or else create it and write on it
            std::ofstream logFile(logFileName, std::ios::app);
            if (!logFile.good()) {
                std::cerr << "Error opening log file." << std::endl;
                return "Error opening file\nEnter command or message: ";
            }
            logFile << message.substr(4) << std::endl;
            return ("\033[2K\r" + message.substr(4) + "\nEnter command or message: ");
        }
        else if (message.find("EXIT") == 0)
        {
            flag = true;
            return ("\033[2K\r" + message.substr(5));
        }
        else if (message.find("SV_FULL") == 0)
        {
            flag = true;
            return "Server is currently full";
        }
        else
        {
            return "";
        }
    }

}




bool Client::isConnected() 
{
    return connected;
}
void Client::setSocket(SOCKET newSocket) 
{
    clientSocket = newSocket;
}
SOCKET Client::getSocket() const 
{
    return clientSocket;
}
std::string Client::getUsername() const 
{
    return username;
}
void Client::setUsername(std::string _username)
{
	this->username = _username;
}



