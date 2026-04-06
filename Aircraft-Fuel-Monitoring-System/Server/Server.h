#pragma once

#include <winsock2.h>
#include <string>
#include <vector>
#include "../Shared/DataPacket.h"
#include "StateMachine.h"

constexpr const char* AUTH_TOKEN = "FUEL_SYS_SECRET";
constexpr double FUEL_ALERT_THRESHOLD = 2000.0;

class Server {
public:
    explicit Server(int port, const std::string& logFile = "server_log.csv");
    ~Server();

    bool initializeWinsock();
    bool startServer();
    void runAcceptLoop();
    void cleanup();

private:
    int         port_;
    SOCKET      listenSocket_;
    std::string logFile_;

    void handleClient(SOCKET clientSock);
    bool authenticateClient(SOCKET clientSock);
    bool receivePacket(SOCKET clientSock, DataPacket& out);
    bool sendLargeDataset(SOCKET clientSock);
    void checkFuelAlert(const DataPacket& packet);
    bool sendAll(SOCKET s, const char* buf, int len);
    bool recvAll(SOCKET s, char* buf, int len);
};