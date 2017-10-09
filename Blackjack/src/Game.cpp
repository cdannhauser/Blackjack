#include "Game.h"

Game::Game()
{
    resetDeck(3); //Use 3 decks
    srand(time(NULL)); //Start the random seed using the current time
}

void Game::resetDeck(unsigned int numberOfDecks){
    deck.clear();
    deck.resize(52*numberOfDecks); //Each deck has 52 cards
    //Populate the deck with cards represented by unsigned integers from 0 to 51
    for(unsigned int i=0; i<numberOfDecks; i++)
        for(unsigned int j=0; j<52; j++)
            deck[52*i+j] = j;
}

//Draw a random card from the deck
unsigned int Game::draw(){
    int index = rand() % deck.size();
    unsigned int card = deck[index];
    deck.erase(deck.begin() + index); //Drawing a card removes it from the deck

    return card;
}

//Deal starting hands to each player
void Game::deal(Server& server){
    //Player[0] is always the dealer
    players[0].value = 0; //Hand value starts at 0
    players[0].clearHand(); //Hand starts empty

    unsigned int card = draw();
    screen << "Dealer's revealed card: ";
    //Each card is an unsigned int from 0 to 51. translateCard() extracts the
    //value of the card and prints its actual name (i.e 51 is King of spades)
    players[0].value += translateCard(card);
    players[0].dealCard(card); //Dealer starts with one card
    screen << std::endl << "Value: " << players[0].value << std::endl;

    //Deal two cards to each player
    for(unsigned int i=1; i<players.size(); i++){
        players[i].value = 0;
        players[i].clearHand();

        card = draw();
        screen << players[i].name << "'s cards: ";
        players[i].value += translateCard(card);
        players[i].dealCard(card);

        card = draw();
        screen << ", ";
        players[i].value += translateCard(card);
        players[i].dealCard(card);

        //Display the value of the player's hand
        screen << std::endl << "Value: " << players[i].value << std::endl;
    }

    //Flush the screen stream to the console and each client
    std::cout << screen.str();
    for(unsigned int i=1; i<players.size(); i++)
        server.sendRawData(i-1, screen.str());
    screen.str(""); //Clear the stream
}

//Deal one card to a specified player
void Game::dealOne(Server& server, unsigned int player){
    players[player].value = 0;

    unsigned int card = draw();
    players[player].dealCard(card);
    screen << players[player].name << "'s cards: ";

    //Display all of the players cards and sum up their values
    for(unsigned int i=0; i<players[player].hand.size(); i++){
        players[player].value += translateCard(players[player].hand[i]);
        if(i != players[player].hand.size() - 1) screen << ", ";
    }

    screen << std::endl << "Value: " << players[player].value << std::endl;

    std::cout << screen.str();
    for(unsigned int i=1; i<players.size(); i++)
        server.sendRawData(i-1, screen.str());
    screen.str("");
}

//Obtain bets from each player
void Game::getBets(Server& server){

    //Send a <b> command to each client requesting that they place a bet
    for(unsigned int i=1; i<players.size(); i++){
        players[i].placedBet = false;
        server.requestBet(i-1, players[i].money);
    }

    std::cout << "Waiting for all players to place bets...\n\n";

    //Block execution until each player has placed a bet. Bets are placed
    //concurrently in separate threads
    bool allPlayersReady = false;
    while(!allPlayersReady){
        allPlayersReady = true;
        for(unsigned int i=1; i<players.size(); i++){
            if(!players[i].placedBet){
                parseInput(i-1, server); //Parse through the client input buffer
                                         //for an <n> command
                if(!players[i].placedBet) allPlayersReady = false;
            }
        }
    }
}

//Resolve a bet after the player responds
void Game::placeBet(Server& server, unsigned int player){
    players[player].placedBet = true;

    //Display the player's name and the amount that they bet
    screen << players[player].name << " bet $" << players[player].bet << std::endl;
    std::cout << screen.str();
    for(unsigned int i=1; i<players.size(); i++)
        server.sendRawData(i-1, screen.str());
    screen.str("");
}

//Determine if a specified player would like to hit or stand
void Game::hitOrStand(Server& server, unsigned int player){
    players[player].stand = false;

    //Player can keep hitting until they obtain a value of at least 21 or choose
    //to stand
    while(players[player].value < 21 && !players[player].stand){
        //Send an <h> command to the client
        server.requestHit(player-1);

        //Block execution until the player decides
        players[player].acceptedHitOrStand = false;
        while(!players[player].acceptedHitOrStand){
            parseInput(player-1, server); //Parse the input buffer for an <h> command
        }

        //Display player status if they hit or exceed 21
        if(players[player].value > 21) screen << players[player].name << " busted!\n";
        else if(players[player].value == 21) screen << players[player].name << " has 21!\n";

        if(players[player].stand)
            screen << players[player].name << " has chosen to stand\n";

        if(screen.str() != ""){
            std::cout << screen.str();
            for(unsigned int i=1; i<players.size(); i++)
                server.sendRawData(i-1, screen.str());
            screen.str("");
        }
    }
}

//Adjust the funds of players according to their bets and values
void Game::payout(Server& server){
    for(unsigned int i=1; i<players.size(); i++){
        //Loss condition
        if(players[i].value > 21 || (players[i].value < players[0].value && !(players[0].value > 21))){
            players[i].money -= players[i].bet;
            screen << players[i].name << " lost $" << players[i].bet << std::endl;
        }
        //Win condition
        else {
            players[i].money += players[i].bet;
            screen << players[i].name << " gained $" << players[i].bet << std::endl;
        }
    }

    screen << "\n\n";

    std::cout << screen.str();
    for(unsigned int i=1; i<players.size(); i++){
        server.sendRawData(i-1, screen.str());
    }
    screen.str("");

    //Eliminate players who are out of money
    for(unsigned int i=1; i<players.size(); i++){
        if(players[i].money <= 0){
            server.eliminate(i-1);
            players.erase(players.begin() + i);
        }
    }
}

//Parse through the input buffer and react to commands. Remove commands after
//viewing them
void Game::parseInput(unsigned int client, Server& server){
    std::list<char*>::iterator itr = server.clientReceiveBuffer[client].begin();
    while(itr != server.clientReceiveBuffer[client].end()){
        std::string command = *itr;

        //All commands start with the '<' character
        if(command[0] == '<')
            switch(command[1]){
                //Name command (<n>name). Sets player name
                case 'n':
                    players[client+1].name = command.substr(3);
                    break;
                //Bet command (<b>bet). Sets player bet
                case 'b':
                    players[client+1].bet = atoi(command.substr(3).c_str());
                    placeBet(server, client+1);
                    break;
                //Hit or stand command (<h>h/s). Hit draws a card for the player
                //and stand simply passes to the next turn
                case 'h':
                    players[client+1].acceptedHitOrStand = true;
                    if(command[3] == 'h') dealOne(server, client+1);
                    else if(command[3] == 's') players[client+1].stand = true;
                    break;
            }
        server.clientReceiveBuffer[client].erase(itr++);
    }
}

//Initiate a game of Blackjack!
void Game::start(){

    //Start the server
    Server server;
    server.start();

    //Signify when all users have joined by pressing enter
    std::cout << "Press enter after all players have joined.\n";
    std::cin.get();

    //Add a dealer and an additional player object for each client
    players.clear();
    players.push_back(Player("Dealer"));
    while(players.size() < server.getClientCount() + 1){

        players.push_back(Player());
    }

    std::cout << "Waiting for all players to input name...\n\n";

    //Block execution until each player has entered a name
    bool allPlayersReady = false;
    while(!allPlayersReady){
        allPlayersReady = true;
        for(unsigned int i=1; i<players.size(); i++){
            if(players[i].name == ""){
                parseInput(i-1, server); //Parse input buffer for <n> command
                if(players[i].name == "") allPlayersReady = false;
            }
        }
    }

    //List all of the connected players
    std::cout << "Players: ";
    for(unsigned int i=0; i<players.size(); i++){
        std::cout << players[i].name;
        if(i != players.size()-1) std::cout << ", ";
    }
    std::cout << std::endl;

    //Start the game loop
    while(players.size() > 1)
        loop(server);

    std::cin.get();
    server.stop();
}

void Game::loop(Server& server){
    getBets(server); //Get each player's bet
    deal(server); //Deal out starting hands

    //Sequentially ask each player to hit or stand
    for(unsigned int i=1; i<players.size(); i++)
        hitOrStand(server, i);

    //Dealer reveals one card. If the dealer's total is <= 16, they must
    //draw another card (repeating)
    screen << std::string(40, '-') << std::endl;
    dealOne(server, 0);
    while(players[0].value <= 16){
        screen << "Dealer must take another card\n";
        dealOne(server, 0);
    }

    std::cout << screen.str();
    for(unsigned int i=1; i<players.size(); i++)
        server.sendRawData(i-1, screen.str());
    screen.str("");

    payout(server); //Resolve payouts for each player
}

//Translate a card from unsigned integer to useful data
unsigned int Game::translateCard(unsigned int card){
    unsigned int value = card % 13; //Value from 0 to 12
    unsigned int suit  = card / 13; //Suite from 0 to 3

    //Display actual name of card and adjust the value to be correct
    switch(value){
        case 0:
            screen << "Ace";
            value = 11;
            break;
        case 10:
            screen << "Jack";
            value = 10;
            break;
        case 11:
            screen << "Queen";
            value = 10;
            break;
        case 12:
            screen << "King";
            value = 10;
            break;
        default:
            value++;
            screen << value;
            break;
    }

    screen << " of ";

    //Display actual suit of the card
    switch(suit){
        case 0:
            screen << "diamonds";
            break;
        case 1:
            screen << "clubs";
            break;
        case 2:
            screen << "hearts";
            break;
        case 3:
            screen << "spades";
            break;
    }

    return value;
}
