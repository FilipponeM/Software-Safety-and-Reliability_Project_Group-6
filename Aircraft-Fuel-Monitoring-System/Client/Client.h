//Client.h
//
//
//

#pragma once

#include <winsock2.h>

//client class
class Client
{
private:

    const char* serverIP;
    int serverPort;
    SOCKET clientSocket;

public:
    Client(const char* ip, int port);
    ~Client();

    bool initializeWinsock();
    bool connectToServer();
    void cleanup();
};