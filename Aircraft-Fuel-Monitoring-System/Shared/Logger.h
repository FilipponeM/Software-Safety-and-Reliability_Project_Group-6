#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <mutex>
#include <chrono>
#include "DataPacket.h"

// REQ-LOG-010: Log all transmitted/received packets
// REQ-LOG-020: Log connection events and errors
// REQ-LOG-030: CSV format
// REQ-LOG-040: Timestamps on every row
class Logger {
public:
    static void logEvent(const std::string& filename,
                         const std::string& eventType,
                         const std::string& detail = "") {
        std::lock_guard<std::mutex> lock(mutex());
        std::ofstream out(filename, std::ios::app);
        if (!out.is_open()) return;
        ensureHeader(out);
        out << nowEpoch() << "," << eventType << ",,,,," << detail << "\n";
    }

    static void logPacket(const std::string& filename,
                          const std::string& eventType,
                          const DataPacket&  packet) {
        std::lock_guard<std::mutex> lock(mutex());
        std::ofstream out(filename, std::ios::app);
        if (!out.is_open()) return;
        ensureHeader(out);

        std::string ext;
        if (packet.extensionData &&
            packet.header.payloadSize > sizeof(TelemetryData)) {
            size_t n = packet.header.payloadSize - sizeof(TelemetryData);
            ext = std::string(packet.extensionData, n);
            for (char& c : ext)
                if (c == ',' || c == '\n' || c == '\r') c = ' ';
        }

        out << packet.header.timestamp              << ","
            << eventType                            << ","
            << packet.header.aircraftID             << ","
            << packet.telemetry.fuelLevel           << ","
            << packet.telemetry.fuelConsumptionRate << ","
            << packet.telemetry.fuelTemperature     << ","
            << ext                                  << "\n";
    }

    static void logError(const std::string& filename,
                         const std::string& errorMsg) {
        logEvent(filename, "ERROR", errorMsg);
    }

    static void logStateTransition(const std::string& filename,
                                   const std::string& fromState,
                                   const std::string& toState) {
        logEvent(filename, "STATE_TRANSITION", fromState + "->" + toState);
    }

private:
    static std::mutex& mutex() {
        static std::mutex m;
        return m;
    }

    static long long nowEpoch() {
        return std::chrono::system_clock::to_time_t(
            std::chrono::system_clock::now());
    }

    static void ensureHeader(std::ofstream& out) {
        if (out.tellp() == 0)
            out << "Timestamp,EventType,AircraftID,"
                   "FuelLevel,ConsumptionRate,Temperature,Detail\n";
    }
};