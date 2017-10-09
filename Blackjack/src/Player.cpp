#include "Player.h"

Player::Player()
{
    clearHand();
}

Player::Player(std::string _name): name(_name)
{
    clearHand();
}

void Player::clearHand(){
    hand.clear();
}

void Player::dealCard(unsigned int card){
    hand.push_back(card);
}
