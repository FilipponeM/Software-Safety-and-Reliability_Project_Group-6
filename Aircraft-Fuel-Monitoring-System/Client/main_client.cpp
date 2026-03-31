//main_client.cpp

#include <iostream>
#include <chrono>
#include <thread>
#include "Client.h"
#include "../Shared/DataPacket.h"
#include "../Shared/Logger.h"

int main()
{
    Client client("127.0.0.1", 8080);

    if (!client.initializeWinsock())
    {
        return 1;
    }

    if (!client.connectToServer())
    {
        return 2;
    }

    std::cout << "Client setup complete.\n";
    Logger::logEvent("client_log.csv", "CONNECT");

    for (int i = 0; i < 10; ++i) {
        DataPacket packet;
        strcpy_s(packet.header.aircraftID, sizeof(packet.header.aircraftID), "FLIGHT-101");
        packet.header.timestamp = static_cast<uint32_t>(std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()));
        packet.telemetry.fuelLevel = 10000.0 - (i * 150.0);
        packet.telemetry.fuelConsumptionRate = 150.0;
        packet.telemetry.fuelTemperature = 25.0 + (i * 0.5);
        
        std::string ext = "Status: OK; Iteration: " + std::to_string(i);
        packet.setExtensionData(ext.c_str(), ext.length());

        if (client.sendDataPacket(packet)) {
            std::cout << "Sent packet " << i + 1 << " (Aircraft: " << packet.header.aircraftID << ", Fuel: " << packet.telemetry.fuelLevel << ")\n";
            Logger::logPacket("client_log.csv", "SEND", packet);
        } else {
            break;
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    std::cout << "Disconnecting...\n";
    Logger::logEvent("client_log.csv", "DISCONNECT");
    return 0;
}