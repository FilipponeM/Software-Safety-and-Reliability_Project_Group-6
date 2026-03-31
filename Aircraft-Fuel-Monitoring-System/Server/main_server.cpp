// main_server.cpp
//
//
//


#include <iostream>
#include "Server.h"

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

    return 0;
}