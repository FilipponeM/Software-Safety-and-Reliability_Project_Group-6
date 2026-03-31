// Server.cpp
// 
// 
// 
//

#include "Server.h"
#include <iostream>

#pragma comment(lib, "ws2_32.lib")

Server::Server(int port)
    : port(port), listenSocket(INVALID_SOCKET), clientSocket(INVALID_SOCKET)
{}

Server::~Server()
{
    cleanup();
}

//Starting Winsock
bool Server::initializeWinsock()
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

//tcp connection to client
bool Server::startServer()
{
    listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (listenSocket == INVALID_SOCKET)
    {
        std::cout << "Socket creation failed.\n";
        return false;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(listenSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
    {
        std::cout << "Bind failed.\n";
        return false;
    }

    if (listen(listenSocket, 1) == SOCKET_ERROR)
    {
        std::cout << "Listen failed.\n";
        return false;
    }

    std::cout << "Server is listening on port " << port << ".\n";
    return true;
}

bool Server::acceptClient()
{
    std::cout << "Waiting for client connection...\n";

    clientSocket = accept(listenSocket, nullptr, nullptr);

    if (clientSocket == INVALID_SOCKET)
    {
        std::cout << "Accept failed.\n";
        return false;
    }

    std::cout << "Client connected successfully.\n";
    return true;
}

void Server::cleanup()
{
    if (clientSocket != INVALID_SOCKET)
    {
        closesocket(clientSocket);
        clientSocket = INVALID_SOCKET;
    }

    if (listenSocket != INVALID_SOCKET)
    {
        closesocket(listenSocket);
        listenSocket = INVALID_SOCKET;
    }

    WSACleanup();
}