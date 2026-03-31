//Client.cpp

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include "Client.h"

#pragma comment(lib, "ws2_32.lib")

Client::Client(const char* ip, int port)
    : serverIP(ip), serverPort(port), clientSocket(INVALID_SOCKET)
{
}

Client::~Client()
{
    cleanup();
}

bool Client::initializeWinsock()
{
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);

    if (result != 0)
    {
        std::cout << "WSAStartup failed.\n";
        return false;
    }

    std::cout << "Winsock initialized.\n";
    return true;
}

bool Client::connectToServer()
{
    clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (clientSocket == INVALID_SOCKET)
    {
        std::cout << "Socket creation failed.\n";
        return false;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(serverPort);
    serverAddr.sin_addr.s_addr = inet_addr(serverIP);

    if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
    {
        std::cout << "Connection to server failed.\n";
        return false;
    }

    std::cout << "Connected to server successfully.\n";
    return true;
}

bool Client::sendDataPacket(const DataPacket& packet)
{
    if (clientSocket == INVALID_SOCKET) {
        std::cout << "Socket is not connected.\n";
        return false;
    }
    
    std::vector<char> buffer = packet.serialize();
    int result = send(clientSocket, buffer.data(), static_cast<int>(buffer.size()), 0);
    if (result == SOCKET_ERROR) {
        std::cout << "Send failed with error: " << WSAGetLastError() << "\n";
        return false;
    }
    return true;
}

void Client::cleanup()
{
    if (clientSocket != INVALID_SOCKET)
    {
        closesocket(clientSocket);
        clientSocket = INVALID_SOCKET;
    }

    WSACleanup();
}