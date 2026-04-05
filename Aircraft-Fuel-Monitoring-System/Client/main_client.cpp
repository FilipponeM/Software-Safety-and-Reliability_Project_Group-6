#include <iostream>
#include <chrono>
#include <thread>
#include <iomanip>
#include "Client.h"
#include "../Shared/DataPacket.h"
#include "../Shared/Logger.h"

constexpr double FUEL_ALERT_THRESHOLD = 2000.0;

// REQ-CLT-070: user-friendly console display
void displayTelemetry(const DataPacket& packet, int i) {
    std::cout << "\n+-------------------------------------+\n";
    std::cout << "|  AIRCRAFT FUEL MONITOR  [Pkt #"
        << std::setw(3) << (i + 1) << "]  |\n";
    std::cout << "+-------------------------------------+\n";
    std::cout << "| Aircraft : " << std::left << std::setw(26)
        << packet.header.aircraftID << "|\n";
    std::cout << "| Fuel     : " << std::setw(23) << std::fixed
        << std::setprecision(1) << packet.telemetry.fuelLevel << "|\n";
    std::cout << "| Rate     : " << std::setw(23)
        << packet.telemetry.fuelConsumptionRate << "|\n";
    std::cout << "| Temp     : " << std::setw(23)
        << packet.telemetry.fuelTemperature << "|\n";
    // REQ-CLT-080: visual alert
    if (packet.telemetry.fuelLevel < FUEL_ALERT_THRESHOLD)
        std::cout << "|  *** LOW FUEL WARNING ***           |\n";
    std::cout << "+-------------------------------------+\n";
}

int main() {
    std::cout << "=== Aircraft Fuel Monitoring System - Client ===\n\n";

    Client client("127.0.0.1", 8080, "client_log.csv");
    if (!client.initializeWinsock()) return 1;

    // REQ-CLT-030 / REQ-COM-030: connect with retry
    if (!client.connectToServer()) return 2;
    Logger::logEvent("client_log.csv", "CONNECT");

    // REQ-SVR-020: authenticate
    if (!client.authenticate("FUEL_SYS_SECRET")) {
        Logger::logError("client_log.csv", "Authentication failed");
        return 3;
    }

    for (int i = 0; i < 10; ++i) {
        DataPacket packet;
        strcpy_s(packet.header.aircraftID, sizeof(packet.header.aircraftID), "FLIGHT-101");
        packet.header.timestamp = static_cast<uint32_t>(
            std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()));
        packet.header.packetType = PKT_TELEMETRY;
        packet.telemetry.fuelLevel = 10000.0 - (i * 150.0);
        packet.telemetry.fuelConsumptionRate = 150.0;
        packet.telemetry.fuelTemperature = 25.0 + (i * 0.5);

        std::string ext = "Status:OK;Iter:" + std::to_string(i);
        packet.setExtensionData(ext.c_str(), ext.size());

        // REQ-CLT-070 / REQ-CLT-080
        displayTelemetry(packet, i);

        if (client.sendDataPacket(packet)) {
            // REQ-CLT-050 / REQ-LOG-010
            Logger::logPacket("client_log.csv", "SEND", packet);
        }
        else {
            Logger::logError("client_log.csv", "Send failed on packet " + std::to_string(i));
            // REQ-COM-030: reconnect
            client.cleanup();
            if (client.connectToServer() && client.authenticate("FUEL_SYS_SECRET"))
                client.sendDataPacket(packet);
        }
        // REQ-CLT-040: ~1 second interval
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // REQ-CLT-060: request large dataset
    std::cout << "\n[CLIENT] Requesting dataset from server...\n";
    std::vector<char> dataset;
    if (client.requestAndReceiveLargeDataset(dataset))
        std::cout << "[CLIENT] Got " << dataset.size() << " bytes.\n";

    Logger::logEvent("client_log.csv", "DISCONNECT");
    std::cout << "\n[CLIENT] Done.\n";
    return 0;
}