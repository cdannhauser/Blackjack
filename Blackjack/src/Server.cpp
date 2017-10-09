#include "Server.h"

Server::Server()
{
    // Initialize Winsock
    int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) {
        std::cout << "WSAStartup failed with error " << iResult << std::endl;
        exit(1);
    }

    //Initialize all containers to accommodate up to 5 clients
    clientSockets.resize(5);
    clientThreads.resize(5);
    clientReady.resize(5);

    for(unsigned int i=0; i<5; i++){
        clientReady[i] = true; //Determines if client is ready to receive data. Initially true
    }
}

//Create server-side socket
void Server::createSocket(){
    //Set socket settings
    ZeroMemory(&hints, sizeof (hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve the local address and port to be used by the server
    int iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
    if (iResult != 0) error(iResult, "Create socket failed with error ");

    // Create a SOCKET for the server to listen for client connections
    listenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

    if (listenSocket == INVALID_SOCKET) error(WSAGetLastError(), "Invalid socket error ");
}

//Bind server-side socket
void Server::bindSocket(){
    // Setup the TCP listening socket
    int iResult = bind( listenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        closesocket(listenSocket);
        freeaddrinfo(result);
        error(iResult, "Bind failed with error ");
    }
    //Info no longer needed, can be freed
    freeaddrinfo(result);
}

//Start listening for client connections
void Server::startListen(){
    if ( listen( listenSocket, SOMAXCONN ) == SOCKET_ERROR ) {
        closesocket(listenSocket);
        error(WSAGetLastError(), "Listen failed with error ");
    }
}

//Accept a client connection. Never called from the main thread
//because accept() blocks until a client connects
void Server::acceptClient(){
    // Accept a client socket
    clientSockets[clientCount] = accept(listenSocket, NULL, NULL);
    if(serverRunning == false) std::cout << "Server stopped\n"; //Break connection if the server shuts down
    else if (clientSockets[clientCount] == INVALID_SOCKET) {
        closesocket(listenSocket);
        error(WSAGetLastError(), "Accept failed with error ");
    }
    else{
        //Client being added. Start new thread to receive input from the client
        clientThreads[clientCount] = std::thread(&Server::handleData, this, clientCount);
        clientCount++;
    }
}

//Wait until a client connects and repeat as long as the server is active.
//This function runs in its own thread always
void Server::waitForClients(){
    while(serverRunning){
        acceptClient();
    }
    //Disconnect all clients and join all client threads on server shutdown
    for(unsigned i=0; i<clientCount; i++){
        closesocket(clientSockets[i]);
        clientThreads[i].join();
    }
}

//Display an error message and stop execution
void Server::error(int errorType, const char* errorMessage){
    std::cout << errorMessage << errorType << std::endl;
    WSACleanup();
    exit(1);
}

//Start running a server
void Server::start(){
    createSocket();
    bindSocket();
    startListen();

    serverRunning = true;
    //Start a new thread to accept clients
    waitForClientsThread = std::thread(&Server::waitForClients, this);
}

//Stop running the server
void Server::stop(){
    closesocket(listenSocket);
    serverRunning = false;
    waitForClientsThread.join();
    clientCount = 0;
    WSACleanup();
}

//Handle input received from a client
void Server::handleData(unsigned int client){

    char recvbuf[DEFAULT_BUFLEN];
    int iResult; //Length of received string
    int recvbuflen = DEFAULT_BUFLEN;

    //Request that the client enter a name before proceeding
    requestName(client);

    // Receive until the client shuts down the connection
    do {
        //Block execution until input is received from the client
        iResult = recv(clientSockets[client], recvbuf, recvbuflen, 0);

        //Stop receiving input when the server shuts down
        if(!serverRunning){
            std::cout << "Client " << (int)client << "disconnected due to server close" << std::endl;
            break;
        }

        //Received a string
        if (iResult > 0) {
            //Lock to ensure thread safety
            dataLock.lock();
            clientReady[client] = true; //Client is ready to receive output now
            clientReceiveBuffer[client].push_back(recvbuf); //Store input command in a public buffer
            dataLock.unlock();

        }
        //Client disconnected
        else if (iResult == 0)
            std::cout << "Connection closing..." << std::endl;
        else {
            closesocket(clientSockets[client]);
            error(WSAGetLastError(), "Receive data failed with error ");
        }

    } while (iResult > 0);

}

//Gets client count!
unsigned int Server::getClientCount(){
    return clientCount;
}

//Send command to client side to request a name entry
void Server::requestName(unsigned int client){
    while(!clientReady[client]); //Wait for client to be ready to receive
    dataLock.lock(); //Lock for thread safety
    clientReady[client] = false;
    dataLock.unlock();
    //<> denotes a command. <n> for name
    int iSendResult = send(clientSockets[client], "<n>", 3, 0);
    if (iSendResult == SOCKET_ERROR) {
        closesocket(clientSockets[client]);
        error(WSAGetLastError(), "Send data failed with error ");
    }
}

//Send command to client side to request a bet entry
void Server::requestBet(unsigned int client, int money){
    //This command requires additional data - the client must know their available funds
    std::stringstream dataStream;
    dataStream << "<b>" << money;
    std::string data = dataStream.str();

    while(!clientReady[client]);
    dataLock.lock();
    clientReady[client] = false;
    dataLock.unlock();
    //Sends command in the format <b>xxx, where xxx represents the client's available funds
    int iSendResult = send(clientSockets[client], data.c_str(), data.length(), 0);
    if (iSendResult == SOCKET_ERROR) {
        closesocket(clientSockets[client]);
        error(WSAGetLastError(), "Send data failed with error ");
    }
}

//Send command to client side to request a hit or stand decision
void Server::requestHit(unsigned int client){
    while(!clientReady[client]);
    dataLock.lock();
    clientReady[client] = false;
    dataLock.unlock();
    //<h> for hit
    int iSendResult = send(clientSockets[client], "<h>", 3, 0);
    if (iSendResult == SOCKET_ERROR) {
        closesocket(clientSockets[client]);
        error(WSAGetLastError(), "Send data failed with error ");
    }
}

//Request that the client kindly disconnect itself as it is now broke
void Server::eliminate(unsigned int client){
    while(!clientReady[client]);
    dataLock.lock();
    clientReady[client] = false;
    dataLock.unlock();
    //<e> for eliminate
    int iSendResult = send(clientSockets[client], "<e>", 3, 0);
    if (iSendResult == SOCKET_ERROR) {
        closesocket(clientSockets[client]);
        error(WSAGetLastError(), "Send data failed with error ");
    }

    //Wait for client to process prior request
    while(!clientReady[client]);

    //Remove all traces of the client from the containers
    clientThreads.erase(clientThreads.begin()+client);
    clientThreads.push_back(std::thread());

    closesocket(clientSockets[client]);
    clientSockets.erase(clientSockets.begin()+client);
    clientSockets.push_back(SOCKET());

    clientReady.erase(clientReady.begin()+client);
    clientReady.push_back(bool());

    clientCount--;
}

//Send a raw string to the client. No command associated with the string
void Server::sendRawData(unsigned int client, std::string data){
    int overrideCount = 0; //Workaround variable. The loop was occasionally stuck for this function
    while(!clientReady[client] && overrideCount < 10) overrideCount++;
    dataLock.lock();
    clientReady[client] = false;
    dataLock.unlock();

    int iSendResult = send(clientSockets[client], data.c_str(), data.length(), 0);
    if (iSendResult == SOCKET_ERROR) {
        closesocket(clientSockets[client]);
        error(WSAGetLastError(), "Send data failed with error ");
    }
}
