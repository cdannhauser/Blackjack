#ifndef PLAYER_H
#define PLAYER_H

#include <vector>
#include <string>
#include <iostream>

//Entirely public class representing a single player. Mostly used for
//convenience of storing player data

class Player
{
    public:

        std::string name;
        unsigned int value;
        std::vector<unsigned int> hand;
        int money = 100;
        bool placedBet, acceptedHitOrStand, stand;
        unsigned int bet;

        Player();
        Player(std::string _name);

        void clearHand();
        void dealCard(unsigned int card);

};

#endif // PLAYER_H
