#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <mutex>
#include <chrono>
#include "DataPacket.h"

class Logger {
public:
    static void logEvent(const std::string& filename, const std::string& eventType) {
        static std::mutex logMutex;
        std::lock_guard<std::mutex> lock(logMutex);
        
        bool isNewFile = false;
        std::ifstream testFile(filename);
        if (!testFile.good()) {
            isNewFile = true;
        }
        testFile.close();

        std::ofstream out(filename, std::ios::app);
        if (out.is_open()) {
            if (isNewFile) {
                out << "Timestamp,EventType,AircraftID,FuelLevel,ConsumptionRate,Temperature,ExtensionData\n";
            }
            auto now = std::chrono::system_clock::now();
            auto time = std::chrono::system_clock::to_time_t(now);
            out << time << "," << eventType << ",,,,,\n";
        }
    }

    static void logPacket(const std::string& filename, const std::string& eventType, const DataPacket& packet) {
        static std::mutex logMutex;
        std::lock_guard<std::mutex> lock(logMutex);
        
        bool isNewFile = false;
        std::ifstream testFile(filename);
        if (!testFile.good()) {
            isNewFile = true;
        }
        testFile.close();

        std::ofstream out(filename, std::ios::app);
        if (out.is_open()) {
            if (isNewFile) {
                out << "Timestamp,EventType,AircraftID,FuelLevel,ConsumptionRate,Temperature,ExtensionData\n";
            }
            
            std::string extData = "";
            if (packet.extensionData && packet.header.payloadSize > sizeof(TelemetryData)) {
                size_t extSize = packet.header.payloadSize - sizeof(TelemetryData);
                // Assume extension is a null-terminated string or we just make it hex
                // For simplicity, treat as string up to extSize
                extData = std::string(packet.extensionData, extSize);
                // Remove commas from extension data to not break CSV
                for (char& c : extData) {
                    if (c == ',' || c == '\n' || c == '\r') c = ' ';
                }
            }
            
            out << packet.header.timestamp << "," 
                << eventType << ","
                << packet.header.aircraftID << ","
                << packet.telemetry.fuelLevel << ","
                << packet.telemetry.fuelConsumptionRate << ","
                << packet.telemetry.fuelTemperature << ","
                << extData << "\n";
        }
    }
};
