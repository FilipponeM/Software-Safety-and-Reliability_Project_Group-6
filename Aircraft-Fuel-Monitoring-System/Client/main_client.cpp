//main_client.cpp

#include <iostream>
#include "Client.h"

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

    return 0;
}