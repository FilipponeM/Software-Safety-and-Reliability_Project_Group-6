// main_server.cpp
//
//
//


#include <iostream>
#include "Server.h"
#include "../Shared/DataPacket.h"
#include "../Shared/Logger.h"

//main function to run the server with error handling for the different points during connection
int main()
{

    //create server class at port 8080 (temp)
    Server server(8080);

    if (!server.initializeWinsock())
    {
        return 1;
    }

    if (!server.startServer())
    {
        return 2;
    }

    if (!server.acceptClient())
    {
        return 3;
    }

    std::cout << "Server setup complete.\n";
    Logger::logEvent("server_log.csv", "CONNECT");

    while (true) {
        DataPacket packet;
        if (server.receiveData(packet)) {
            std::cout << "Received packet (Aircraft: " << packet.header.aircraftID << ", Fuel: " << packet.telemetry.fuelLevel << ")\n";
            Logger::logPacket("server_log.csv", "RECEIVE", packet);
        } else {
            std::cout << "Client disconnected or error.\n";
            break;
        }
    }

    Logger::logEvent("server_log.csv", "DISCONNECT");
    return 0;
}