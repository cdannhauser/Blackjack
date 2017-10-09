/*
|*********************************|
|--- LAN Blackjack Game Client ---|
|-- written by Chris Dannhauser --|
|*********************************|
*/

#include <iostream>
#include "Client.h"

int main(){

    Client client;

    //Ask user for the server IP address. Typing "local" defaults
    //to localhost address
    std::string hostName;
    std::cout << "Please enter the IP address of the desired server: ";
    std::cin >> hostName;
    if(hostName == "local") hostName = "127.0.0.1";

    //Start client and attempt to connect to the specified IP address
    client.connectToServer(hostName.c_str());

    std::cin.get();
    std::cin.get();

    return 0;
}

