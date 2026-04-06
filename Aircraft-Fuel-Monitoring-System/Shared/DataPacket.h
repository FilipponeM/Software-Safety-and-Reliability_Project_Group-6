#pragma once

#include <string>
#include <vector>
#include <cstring>
#include <cstdint>

struct PacketHeader {
    char     aircraftID[16];
    uint32_t payloadSize;
    uint32_t timestamp;
    uint8_t  packetType;
    uint8_t  _pad[3];
};

struct TelemetryData {
    double fuelLevel;
    double fuelConsumptionRate;
    double fuelTemperature;
};

constexpr uint8_t PKT_TELEMETRY     = 1;
constexpr uint8_t PKT_AUTH_REQUEST  = 2;
constexpr uint8_t PKT_AUTH_ACK      = 3;
constexpr uint8_t PKT_DATA_REQUEST  = 4;
constexpr uint8_t PKT_DATA_RESPONSE = 5;

class DataPacket {
public:
    PacketHeader  header;
    TelemetryData telemetry;
    char*         extensionData;

    DataPacket() : extensionData(nullptr) {
        memset(&header,    0, sizeof(PacketHeader));
        memset(&telemetry, 0, sizeof(TelemetryData));
        header.payloadSize = sizeof(TelemetryData);
        header.packetType  = PKT_TELEMETRY;
    }

    ~DataPacket() {
        delete[] extensionData;
        extensionData = nullptr;
    }

    DataPacket(const DataPacket& other) {
        header    = other.header;
        telemetry = other.telemetry;
        if (other.extensionData && header.payloadSize > sizeof(TelemetryData)) {
            size_t n = header.payloadSize - sizeof(TelemetryData);
            extensionData = new char[n];
            memcpy(extensionData, other.extensionData, n);
        } else {
            extensionData = nullptr;
        }
    }

    DataPacket& operator=(const DataPacket& other) {
        if (this != &other) {
            delete[] extensionData;
            header    = other.header;
            telemetry = other.telemetry;
            if (other.extensionData && header.payloadSize > sizeof(TelemetryData)) {
                size_t n = header.payloadSize - sizeof(TelemetryData);
                extensionData = new char[n];
                memcpy(extensionData, other.extensionData, n);
            } else {
                extensionData = nullptr;
            }
        }
        return *this;
    }

    void setExtensionData(const char* data, size_t size) {
        delete[] extensionData;
        extensionData = nullptr;
        if (size > 0 && data) {
            extensionData = new char[size];
            memcpy(extensionData, data, size);
        }
        header.payloadSize = sizeof(TelemetryData) + static_cast<uint32_t>(size);
    }

    std::vector<char> serialize() const {
        std::vector<char> buf(sizeof(PacketHeader) + header.payloadSize);
        memcpy(buf.data(), &header, sizeof(PacketHeader));
        if (header.payloadSize >= sizeof(TelemetryData))
            memcpy(buf.data() + sizeof(PacketHeader), &telemetry, sizeof(TelemetryData));
        if (extensionData && header.payloadSize > sizeof(TelemetryData)) {
            size_t n = header.payloadSize - sizeof(TelemetryData);
            memcpy(buf.data() + sizeof(PacketHeader) + sizeof(TelemetryData), extensionData, n);
        }
        return buf;
    }

    bool deserialize(const std::vector<char>& buf) {
        if (buf.size() < sizeof(PacketHeader)) return false;
        memcpy(&header, buf.data(), sizeof(PacketHeader));
        if (buf.size() < sizeof(PacketHeader) + header.payloadSize) return false;
        if (header.payloadSize >= sizeof(TelemetryData)) {
            memcpy(&telemetry, buf.data() + sizeof(PacketHeader), sizeof(TelemetryData));
            size_t n = header.payloadSize - sizeof(TelemetryData);
            if (n > 0) {
                delete[] extensionData;
                extensionData = new char[n];
                memcpy(extensionData,
                       buf.data() + sizeof(PacketHeader) + sizeof(TelemetryData), n);
            }
        }
        return true;
    }
};