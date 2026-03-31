//Client.h
//
//
//

#pragma once

#include <winsock2.h>
#include "../Shared/DataPacket.h"

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
    bool sendDataPacket(const DataPacket& packet);
    void cleanup();
};