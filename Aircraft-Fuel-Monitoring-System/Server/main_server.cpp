#include <iostream>
#include "Server.h"

int main() {
    std::cout << "=== Aircraft Fuel Monitoring System - Server ===\n\n";
    Server server(8080, "server_log.csv");
    if (!server.initializeWinsock()) return 1;
    if (!server.startServer())       return 2;
    server.runAcceptLoop();
    server.cleanup();
    return 0;
}