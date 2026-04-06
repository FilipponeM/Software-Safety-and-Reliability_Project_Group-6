#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <thread>
#include <chrono>
#include "Client.h"
#include "../Shared/Logger.h"

#pragma comment(lib, "ws2_32.lib")

Client::Client(const char* ip, int port, const std::string& logFile)
    : serverIP_(ip), serverPort_(port),
    clientSocket_(INVALID_SOCKET), logFile_(logFile),
    reconnectAttempts_(0) {}

Client::~Client() { cleanup(); }

bool Client::initializeWinsock() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        Logger::logError(logFile_, "WSAStartup failed");
        return false;
    }
    std::cout << "[CLIENT] Winsock initialized.\n";
    return true;
}

bool Client::connectToServer() {
    while (reconnectAttempts_ <= MAX_RECONNECT_ATTEMPTS) {
        if (clientSocket_ != INVALID_SOCKET) {
            closesocket(clientSocket_);
            clientSocket_ = INVALID_SOCKET;
        }
        clientSocket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (clientSocket_ == INVALID_SOCKET) return false;

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(serverPort_);
        addr.sin_addr.s_addr = inet_addr(serverIP_);

        if (connect(clientSocket_, (sockaddr*)&addr, sizeof(addr)) == 0) {
            reconnectAttempts_ = 0;
            std::cout << "[CLIENT] Connected to server.\n";
            return true;
        }
        reconnectAttempts_++;
        std::cerr << "[CLIENT] Connect failed (attempt "
            << reconnectAttempts_ << "/"
            << MAX_RECONNECT_ATTEMPTS << "). Retrying in 2s...\n";
        Logger::logError(logFile_, "Connection attempt " +
            std::to_string(reconnectAttempts_) + " failed");
        if (reconnectAttempts_ <= MAX_RECONNECT_ATTEMPTS)
            std::this_thread::sleep_for(std::chrono::seconds(2));
    }
    Logger::logError(logFile_, "All reconnection attempts exhausted");
    return false;
}

bool Client::authenticate(const std::string& token) {
    // Send token directly as raw bytes with auth header
    PacketHeader hdr{};
    hdr.packetType = PKT_AUTH_REQUEST;
    hdr.payloadSize = static_cast<uint32_t>(token.size());

    std::vector<char> buf(sizeof(PacketHeader) + token.size());
    memcpy(buf.data(), &hdr, sizeof(PacketHeader));
    memcpy(buf.data() + sizeof(PacketHeader), token.c_str(), token.size());

    if (!sendAll(buf.data(), static_cast<int>(buf.size()))) {
        Logger::logError(logFile_, "Failed to send auth packet");
        return false;
    }

    // Read ACK header
    PacketHeader ackHdr{};
    if (!recvAll(reinterpret_cast<char*>(&ackHdr), sizeof(PacketHeader))) {
        Logger::logError(logFile_, "Failed to receive auth ACK");
        return false;
    }
    if (ackHdr.packetType != PKT_AUTH_ACK) return false;

    // Read ACK payload
    std::vector<char> payload(ackHdr.payloadSize);
    if (ackHdr.payloadSize > 0)
        if (!recvAll(payload.data(), ackHdr.payloadSize)) return false;

    std::string reply(payload.begin(), payload.end());
    bool ok = (reply == "OK");
    if (ok)
        std::cout << "[CLIENT] Authentication successful.\n";
    else
        Logger::logError(logFile_, "Authentication rejected by server");
    return ok;
}

bool Client::sendDataPacket(const DataPacket& packet) {
    if (clientSocket_ == INVALID_SOCKET) {
        Logger::logError(logFile_, "sendDataPacket: not connected");
        return false;
    }
    auto buf = packet.serialize();
    if (!sendAll(buf.data(), static_cast<int>(buf.size()))) {
        Logger::logError(logFile_, "Send failed: " +
            std::to_string(WSAGetLastError()));
        return false;
    }
    return true;
}

bool Client::requestAndReceiveLargeDataset(std::vector<char>& outData) {
    if (clientSocket_ == INVALID_SOCKET) return false;

    // Send data request header only
    PacketHeader reqHdr{};
    reqHdr.packetType = PKT_DATA_REQUEST;
    reqHdr.payloadSize = 0;

    std::vector<char> reqBuf(sizeof(PacketHeader));
    memcpy(reqBuf.data(), &reqHdr, sizeof(PacketHeader));
    if (!sendAll(reqBuf.data(), static_cast<int>(reqBuf.size()))) {
        Logger::logError(logFile_, "Failed to send data request");
        return false;
    }

    // Read response header
    PacketHeader respHdr{};
    if (!recvAll(reinterpret_cast<char*>(&respHdr), sizeof(PacketHeader))) {
        Logger::logError(logFile_, "Failed to receive dataset header");
        return false;
    }
    if (respHdr.packetType != PKT_DATA_RESPONSE) {
        Logger::logError(logFile_, "Unexpected packet type in response");
        return false;
    }

    // Read full payload
    outData.resize(respHdr.payloadSize);
    if (!recvAll(outData.data(), respHdr.payloadSize)) {
        Logger::logError(logFile_, "Failed to receive dataset payload");
        return false;
    }

    std::cout << "[CLIENT] Received " << outData.size()
        << " bytes from server.\n";
    return true;
}

bool Client::sendAll(const char* buf, int len) {
    int sent = 0;
    while (sent < len) {
        int r = send(clientSocket_, buf + sent, len - sent, 0);
        if (r == SOCKET_ERROR) return false;
        sent += r;
    }
    return true;
}

bool Client::recvAll(char* buf, int len) {
    int got = 0;
    while (got < len) {
        int r = recv(clientSocket_, buf + got, len - got, 0);
        if (r <= 0) return false;
        got += r;
    }
    return true;
}

void Client::cleanup() {
    if (clientSocket_ != INVALID_SOCKET) {
        closesocket(clientSocket_);
        clientSocket_ = INVALID_SOCKET;
    }
    WSACleanup();
}