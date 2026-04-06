#pragma once

#include <winsock2.h>
#include <string>
#include <vector>
#include "../Shared/DataPacket.h"

constexpr int MAX_RECONNECT_ATTEMPTS = 3;

class Client {
public:
    Client(const char* ip, int port,
        const std::string& logFile = "client_log.csv");
    ~Client();

    bool initializeWinsock();
    bool connectToServer();           // REQ-COM-030: retries on failure
    bool authenticate(const std::string& token);  // REQ-SVR-020
    bool sendDataPacket(const DataPacket& packet);
    bool requestAndReceiveLargeDataset(std::vector<char>& outData); // REQ-CLT-060
    void cleanup();
    bool isConnected() const { return clientSocket_ != INVALID_SOCKET; }

private:
    const char* serverIP_;
    int         serverPort_;
    SOCKET      clientSocket_;
    std::string logFile_;
    int         reconnectAttempts_;

    bool sendAll(const char* buf, int len);
    bool recvAll(char* buf, int len);
};