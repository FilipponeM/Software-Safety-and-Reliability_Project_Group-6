#pragma once

#include <string>
#include <vector>
#include <cstring>
#include <cstdint>

struct PacketHeader {
    char aircraftID[16];
    uint32_t payloadSize;
    uint32_t timestamp;
};

struct TelemetryData {
    double fuelLevel;
    double fuelConsumptionRate;
    double fuelTemperature;
};

class DataPacket {
public:
    PacketHeader header;
    TelemetryData telemetry;
    char* extensionData; // Dynamically allocated element for extensibility

    DataPacket() : extensionData(nullptr) {
        memset(&header, 0, sizeof(PacketHeader));
        memset(&telemetry, 0, sizeof(TelemetryData));
        header.payloadSize = sizeof(TelemetryData);
    }

    ~DataPacket() {
        if (extensionData) {
            delete[] extensionData;
            extensionData = nullptr;
        }
    }

    // Copy constructor required due to dynamic allocation
    DataPacket(const DataPacket& other) {
        header = other.header;
        telemetry = other.telemetry;
        if (other.extensionData && header.payloadSize > sizeof(TelemetryData)) {
            size_t extSize = header.payloadSize - sizeof(TelemetryData);
            extensionData = new char[extSize];
            memcpy(extensionData, other.extensionData, extSize);
        } else {
            extensionData = nullptr;
        }
    }

    // Assignment operator
    DataPacket& operator=(const DataPacket& other) {
        if (this != &other) {
            if (extensionData) {
                delete[] extensionData;
            }
            header = other.header;
            telemetry = other.telemetry;
            if (other.extensionData && header.payloadSize > sizeof(TelemetryData)) {
                size_t extSize = header.payloadSize - sizeof(TelemetryData);
                extensionData = new char[extSize];
                memcpy(extensionData, other.extensionData, extSize);
            } else {
                extensionData = nullptr;
            }
        }
        return *this;
    }

    void setExtensionData(const char* data, size_t size) {
        if (extensionData) {
            delete[] extensionData;
            extensionData = nullptr;
        }
        if (size > 0 && data) {
            extensionData = new char[size];
            memcpy(extensionData, data, size);
        }
        header.payloadSize = sizeof(TelemetryData) + static_cast<uint32_t>(size);
    }

    std::vector<char> serialize() const {
        std::vector<char> buffer(sizeof(PacketHeader) + header.payloadSize);
        memcpy(buffer.data(), &header, sizeof(PacketHeader));
        
        if (header.payloadSize >= sizeof(TelemetryData)) {
            memcpy(buffer.data() + sizeof(PacketHeader), &telemetry, sizeof(TelemetryData));
        }

        if (extensionData && header.payloadSize > sizeof(TelemetryData)) {
            size_t extSize = header.payloadSize - sizeof(TelemetryData);
            memcpy(buffer.data() + sizeof(PacketHeader) + sizeof(TelemetryData), extensionData, extSize);
        }
        return buffer;
    }

    bool deserialize(const std::vector<char>& buffer) {
        if (buffer.size() < sizeof(PacketHeader)) return false;
        memcpy(&header, buffer.data(), sizeof(PacketHeader));
        
        if (buffer.size() < sizeof(PacketHeader) + header.payloadSize) return false;
        
        if (header.payloadSize >= sizeof(TelemetryData)) {
            memcpy(&telemetry, buffer.data() + sizeof(PacketHeader), sizeof(TelemetryData));
            
            size_t extSize = header.payloadSize - sizeof(TelemetryData);
            if (extSize > 0) {
                if (extensionData) {
                    delete[] extensionData;
                }
                extensionData = new char[extSize];
                memcpy(extensionData, buffer.data() + sizeof(PacketHeader) + sizeof(TelemetryData), extSize);
            }
        }
        return true;
    }
};
