#ifndef SERVER_H
#define SERVER_H

#define _WIN32_WINNT 0x501

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>

#include <thread>
#include <mutex>
#include <iostream>
#include <sstream>
#include <list>
#include <vector>

#define DEFAULT_PORT "27015"
#define DEFAULT_BUFLEN 512

//This class handles all of the netcode for the server-side application.
//It utilizes the Winsock API for socket programming, and therefore will
//not compile without the Microsoft Windows SDK.

class Server
{
    //Private variables
    WSADATA wsaData;
    struct addrinfo *result = NULL, *ptr = NULL, hints;
    SOCKET listenSocket = INVALID_SOCKET;

    //Containers for storing client data
    std::vector<SOCKET> clientSockets;
    std::vector<std::thread> clientThreads;
    std::vector<bool> clientReady;

    bool serverRunning = false;
    std::thread waitForClientsThread;
    std::mutex dataLock;
    unsigned int clientCount = 0;

    //Private functions
    void createSocket();
    void bindSocket();
    void startListen();
    void acceptClient();

    void waitForClients();

    void handleData(unsigned int client);

    void error(int errorType, const char* errorMessage);

    public:
        std::list<char*> clientReceiveBuffer[5];

        Server();

        void start();
        void stop();

        unsigned int getClientCount();
        void requestName(unsigned int client);
        void requestBet(unsigned int client, int money);
        void requestHit(unsigned int client);
        void eliminate(unsigned int client);
        void sendRawData(unsigned int client, std::string data);
};

#endif // NETWORK_H
