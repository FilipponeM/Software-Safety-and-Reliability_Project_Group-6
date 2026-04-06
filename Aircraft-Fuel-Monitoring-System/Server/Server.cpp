#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "Server.h"
#include "../Shared/Logger.h"
#include <iostream>
#include <thread>
#include <vector>
#include <string>
#include <sstream>

#pragma comment(lib, "ws2_32.lib")

Server::Server(int port, const std::string& logFile)
    : port_(port), listenSocket_(INVALID_SOCKET), logFile_(logFile) {}

Server::~Server() { cleanup(); }

bool Server::initializeWinsock() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        Logger::logError(logFile_, "WSAStartup failed");
        return false;
    }
    std::cout << "[SERVER] Winsock initialized.\n";
    return true;
}

bool Server::startServer() {
    listenSocket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket_ == INVALID_SOCKET) {
        Logger::logError(logFile_, "Socket creation failed");
        return false;
    }
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port_);
    addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(listenSocket_, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        Logger::logError(logFile_, "Bind failed");
        return false;
    }
    if (listen(listenSocket_, SOMAXCONN) == SOCKET_ERROR) {
        Logger::logError(logFile_, "Listen failed");
        return false;
    }
    std::cout << "[SERVER] Listening on port " << port_ << ".\n";
    return true;
}

void Server::runAcceptLoop() {
    std::cout << "[SERVER] Waiting for clients...\n";
    while (true) {
        SOCKET clientSock = accept(listenSocket_, nullptr, nullptr);
        if (clientSock == INVALID_SOCKET) {
            Logger::logError(logFile_, "accept() failed");
            break;
        }
        std::cout << "[SERVER] New client connected.\n";
        std::thread([this, clientSock]() {
            handleClient(clientSock);
            }).detach();
    }
}

void Server::handleClient(SOCKET clientSock) {
    StateMachine sm(logFile_);
    sm.transition(ServerState::CONNECTED);
    Logger::logEvent(logFile_, "CONNECT");

    if (!authenticateClient(clientSock)) {
        Logger::logError(logFile_, "Authentication failed");
        std::cout << "[SERVER] Authentication failed. Disconnecting.\n";
        closesocket(clientSock);
        sm.transition(ServerState::DISCONNECTED, true);
        return;
    }

    std::cout << "[SERVER] Client authenticated.\n";
    sm.transition(ServerState::AUTHENTICATED);
    sm.transition(ServerState::DATA_RECEIVING);

    while (true) {
        PacketHeader hdr{};
        if (!recvAll(clientSock, reinterpret_cast<char*>(&hdr), sizeof(PacketHeader))) {
            Logger::logError(logFile_, "Client disconnected or recv error");
            std::cout << "[SERVER] Client disconnected.\n";
            break;
        }

        if (hdr.packetType == PKT_DATA_REQUEST) {
            sm.transition(ServerState::DATA_TRANSFER);
            std::cout << "[SERVER] Data request received. Sending dataset.\n";
            sendLargeDataset(clientSock);
            sm.transition(ServerState::DATA_RECEIVING);

        }
        else if (hdr.packetType == PKT_TELEMETRY) {
            std::vector<char> buf(sizeof(PacketHeader) + hdr.payloadSize);
            memcpy(buf.data(), &hdr, sizeof(PacketHeader));
            if (hdr.payloadSize > 0) {
                if (!recvAll(clientSock,
                    buf.data() + sizeof(PacketHeader),
                    hdr.payloadSize)) {
                    Logger::logError(logFile_, "Payload recv error");
                    break;
                }
            }

            DataPacket packet;
            if (!packet.deserialize(buf)) {
                Logger::logError(logFile_, "Deserialization failed");
                continue;
            }

            Logger::logPacket(logFile_, "RECEIVE", packet);
            std::cout << "[SERVER] Recv | Aircraft: " << packet.header.aircraftID
                << " | Fuel: " << packet.telemetry.fuelLevel
                << " | Rate: " << packet.telemetry.fuelConsumptionRate
                << " | Temp: " << packet.telemetry.fuelTemperature << "\n";

            checkFuelAlert(packet);

        }
        else {
            Logger::logError(logFile_, "Unknown packet type: " +
                std::to_string(hdr.packetType));
            if (hdr.payloadSize > 0) {
                std::vector<char> drain(hdr.payloadSize);
                recvAll(clientSock, drain.data(), hdr.payloadSize);
            }
        }
    }

    Logger::logEvent(logFile_, "DISCONNECT");
    sm.transition(ServerState::DISCONNECTED, true);
    closesocket(clientSock);
}

bool Server::authenticateClient(SOCKET clientSock) {
    PacketHeader hdr{};
    if (!recvAll(clientSock, reinterpret_cast<char*>(&hdr), sizeof(PacketHeader)))
        return false;
    if (hdr.packetType != PKT_AUTH_REQUEST || hdr.payloadSize == 0)
        return false;

    std::vector<char> tokenBuf(hdr.payloadSize);
    if (!recvAll(clientSock, tokenBuf.data(), hdr.payloadSize))
        return false;

    std::string receivedToken(tokenBuf.begin(), tokenBuf.end());
    bool ok = (receivedToken == AUTH_TOKEN);

    // Send ACK as raw bytes
    PacketHeader ackHdr{};
    ackHdr.packetType = PKT_AUTH_ACK;
    std::string reply = ok ? "OK" : "FAIL";
    ackHdr.payloadSize = static_cast<uint32_t>(reply.size());

    std::vector<char> ackBuf(sizeof(PacketHeader) + reply.size());
    memcpy(ackBuf.data(), &ackHdr, sizeof(PacketHeader));
    memcpy(ackBuf.data() + sizeof(PacketHeader), reply.c_str(), reply.size());
    sendAll(clientSock, ackBuf.data(), static_cast<int>(ackBuf.size()));

    return ok;
}

bool Server::sendLargeDataset(SOCKET clientSock) {
    const size_t TARGET = 1024 * 1024;
    std::ostringstream oss;
    oss << "Timestamp,AircraftID,FuelLevel,ConsumptionRate,Temperature\n";
    uint32_t ts = 1700000000;
    while (oss.tellp() < static_cast<std::streamoff>(TARGET)) {
        oss << ts++ << ",FLIGHT-101,"
            << (10000.0 - (ts % 1000) * 0.15) << ",150.0,"
            << (25.0 + (ts % 100) * 0.01) << "\n";
    }
    std::string data = oss.str();

    // Send as raw header + data
    PacketHeader respHdr{};
    respHdr.packetType = PKT_DATA_RESPONSE;
    respHdr.payloadSize = static_cast<uint32_t>(data.size());

    std::vector<char> buf(sizeof(PacketHeader) + data.size());
    memcpy(buf.data(), &respHdr, sizeof(PacketHeader));
    memcpy(buf.data() + sizeof(PacketHeader), data.c_str(), data.size());

    std::cout << "[SERVER] Sending " << buf.size() << " bytes of dataset.\n";
    return sendAll(clientSock, buf.data(), static_cast<int>(buf.size()));
}

void Server::checkFuelAlert(const DataPacket& packet) {
    if (packet.telemetry.fuelLevel < FUEL_ALERT_THRESHOLD) {
        std::string msg = "LOW FUEL: " + std::string(packet.header.aircraftID)
            + " fuel=" + std::to_string(packet.telemetry.fuelLevel);
        std::cout << "\n*** " << msg << " ***\n\n";
        Logger::logEvent(logFile_, "FUEL_ALERT", msg);
    }
}

bool Server::sendAll(SOCKET s, const char* buf, int len) {
    int sent = 0;
    while (sent < len) {
        int r = send(s, buf + sent, len - sent, 0);
        if (r == SOCKET_ERROR) {
            Logger::logError(logFile_, "send() error: " +
                std::to_string(WSAGetLastError()));
            return false;
        }
        sent += r;
    }
    return true;
}

bool Server::recvAll(SOCKET s, char* buf, int len) {
    int got = 0;
    while (got < len) {
        int r = recv(s, buf + got, len - got, 0);
        if (r <= 0) return false;
        got += r;
    }
    return true;
}

void Server::cleanup() {
    if (listenSocket_ != INVALID_SOCKET) {
        closesocket(listenSocket_);
        listenSocket_ = INVALID_SOCKET;
    }
    WSACleanup();
}