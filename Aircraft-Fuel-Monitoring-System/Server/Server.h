// Server.h
//
//
//

#pragma once

#include <winsock2.h>

//Server class
class Server
{
private:

    int port;
    SOCKET listenSocket;
    SOCKET clientSocket;

public:
    Server(int port);
    ~Server();

    bool initializeWinsock();
    bool startServer();
    bool acceptClient();
    void cleanup();
};