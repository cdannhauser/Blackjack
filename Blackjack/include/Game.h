#ifndef GAME_H
#define GAME_H

#include "Player.h"
#include "Server.h"

#include <random>
#include <ctime>
#include <sstream>

//This class contains all of the game logic and links the players to the server
//functionality.


class Game
{
    std::vector<Player> players;
    std::vector<unsigned int> deck;
    std::stringstream screen; //Screen variable functions as a shared data stream
                              //between server and client

    void resetDeck(unsigned int numberOfDecks);
    unsigned int draw();
    void deal(Server& server);
    void dealOne(Server& server, unsigned int player);
    void getBets(Server& server);
    void placeBet(Server& server, unsigned int player);
    void hitOrStand(Server& server, unsigned int player);
    void payout(Server& server);

    void parseInput(unsigned int client, Server& server);

    void loop(Server& server);

    unsigned int translateCard(unsigned int card);

    public:
        Game();

        void start();

};

#endif // GAME_H
