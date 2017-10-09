#include "Client.h"

Client::Client()
{
    // Initialize Winsock
    int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) {
        std::cout << "WSAStartup failed with error " << iResult << std::endl;
        exit(1);
    }
}

//Create client-side socket
void Client::createSocket(const char* hostName){
    //Set socket settings
    ZeroMemory( &hints, sizeof(hints) );
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    // Resolve the local address and port to be used by the server
    int iResult = getaddrinfo(hostName, DEFAULT_PORT, &hints, &result);
    if (iResult != 0) error(iResult, "Create socket failed with error ");

    // Attempt to connect to the first address returned by
    // the call to getaddrinfo
    ptr = result;

    // Create a SOCKET for connecting to server
    connectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);

    if (connectSocket == INVALID_SOCKET){
        freeaddrinfo(result);
        error(WSAGetLastError(), "Invalid socket error ");
    }
}

//Start a connection with the server
void Client::startConnection(){
    // Connect to server.
    int iResult = connect( connectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        closesocket(connectSocket);
        connectSocket = INVALID_SOCKET;
    }

    // Free the resources returned by getaddrinfo
    freeaddrinfo(result);

    if (connectSocket == INVALID_SOCKET) error(0, "Unable to connect to server ");
}

//Handle data input from the server as well as output to the server
void Client::sendData(){
    int recvbuflen = DEFAULT_BUFLEN;
    char recvbuf[DEFAULT_BUFLEN];
    int iResult; //Length of received string

    // Send/receive data until the server closes the connection
    do {
        //Receive a string from the server
        iResult = recv(connectSocket, recvbuf, recvbuflen, 0);

        //Received a string
        if (iResult > 0){
            //Upon receiving an <e> command, shut down the client
            if(std::string(recvbuf).substr(0,3) == "<e>"){
                //Ping the server in acknowledgment
                int iResult = send(connectSocket, "<r>", 3, 0);
                if (iResult == SOCKET_ERROR) {
                    closesocket(connectSocket);
                    error(WSAGetLastError(), "Send failed with error ");
                }

                //Stop handling data
                std::cout << "You're out of money!\n";
                break;
            }

            //The '<' character signifies a command in need of parsing
            if(recvbuf[0] == '<') parseInput(recvbuf);
            else{
                //Print raw non-command data
                std::cout.write(recvbuf, iResult);
                std::cout << std::endl;

                //Ping the server in acknowledgment
                int iResult = send(connectSocket, "<r>", 3, 0);
                if (iResult == SOCKET_ERROR) {
                    closesocket(connectSocket);
                    error(WSAGetLastError(), "Send failed with error ");
                }
            }
        }
        //Server disconnected
        else if (iResult == 0)
            std::cout << "Connection closed\n";
        else
            std::cout << "Receive failed with error: " << WSAGetLastError() << std::endl;


    } while (iResult > 0);

     // shutdown the connection
    iResult = shutdown(connectSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        closesocket(connectSocket);
        error(WSAGetLastError(), "Shutdown failed with error ");
    }

}

//Parse a command and respond accordingly
void Client::parseInput(std::string command){
    int money;
    switch(command[1]){
        case 'n':
            setName();
            break;
        case 'b':
            money = atoi(command.substr(3).c_str());
            placeBet(money);
            break;
        case 'h':
            hitOrStand();
            break;
    }
}

//Ask the user to enter a name, then send a name command to the server
void Client::setName(){
    std::string sendbuf;
    std::cout << "Name: ";
    std::cin >> sendbuf;
    sendbuf = "<n>" + sendbuf;

    int iResult = send(connectSocket, sendbuf.c_str(), sendbuf.length(), 0);
    if (iResult == SOCKET_ERROR) {
        closesocket(connectSocket);
        error(WSAGetLastError(), "Send failed with error ");
    }
}

//Ask the user to place a bet, then send a bet command to the server
void Client::placeBet(int money){
    std::string sendbuf;
    int value;

    std::cout << "You have $" << money << " available\n";

    //Repeat until the user enters a valid amount
    do{
        std::cout << "Please place your bet (minimum 10): ";
        std::cin >> sendbuf;
        value = atoi(sendbuf.c_str());
        if(value < 10) std::cout << "Bet must be at least 10\n";
        if(value > money) std::cout << "You don't have enough money\n";
    }while(value < 10 || value > money);

    sendbuf = "<b>" + sendbuf;
    int iResult = send(connectSocket, sendbuf.c_str(), sendbuf.length(), 0);
    if (iResult == SOCKET_ERROR) {
        closesocket(connectSocket);
        error(WSAGetLastError(), "Send failed with error ");
    }
}

//Ask the user to select hit or stand, then send a hit/stand command to the server
void Client::hitOrStand(){
    std::string sendbuf;

    std::cout << "Hit or stand? (type h for hit or s for stand): ";

    //Repeat until the user enters a valid option
    do{
        std::cin >> sendbuf;
        if(sendbuf != "h" && sendbuf != "s")
            std::cout << "Please type either h or s\n";
    }while(sendbuf != "h" && sendbuf != "s");

    sendbuf = "<h>" + sendbuf;
    int iResult = send(connectSocket, sendbuf.c_str(), sendbuf.length(), 0);
    if (iResult == SOCKET_ERROR) {
        closesocket(connectSocket);
        error(WSAGetLastError(), "Send failed with error ");
    }
}

//Display an error message and stop execution
void Client::error(int errorType, const char* errorMessage){
    std::cout << errorMessage << errorType << std::endl;
    WSACleanup();
    exit(1);
}

//Initiate a connection to a server
void Client::connectToServer(const char* hostName){
    createSocket(hostName);
    startConnection();
    sendData();
}
