#ifndef CLIENT_H
#define CLIENT_H

#define _WIN32_WINNT 0x501

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>

#include <thread>
#include <iostream>

#define DEFAULT_PORT "27015"
#define DEFAULT_BUFLEN 512


//This class handles all of the netcode for the client-side application.
//It utilizes the Winsock API for socket programming, and therefore will
//not compile without the Microsoft Windows SDK.

class Client
{
    //Private variables
    WSADATA wsaData;
    struct addrinfo *result = NULL, *ptr = NULL, hints;
    SOCKET connectSocket = INVALID_SOCKET;

    //Private functions
    void createSocket(const char* hostName);
    void startConnection();

    void sendData();
    void parseInput(std::string command);
    void setName();
    void placeBet(int money);
    void hitOrStand();

    void error(int errorType, const char* errorMessage);

    public:
        Client();

        void connectToServer(const char* hostName);
};

#endif // CLIENT_H
