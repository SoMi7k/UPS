#ifndef PLAYER_CPP
#define PLAYER_CPP

#pragma once
#include <vector>
#include <iostream>
#include <string>
#include "Hand.hpp"

class Player {
private:
    int number;
    Hand hand;
    std::string nick;

public:
    explicit Player(int number, std::string nick);
    void removeHand();
    void addCard(const Card& card);
    void addWonCard(const Card& card);
    bool hasCardInHand() const;
    bool hasWon() const;
    Card chooseCard();
    Card play(CardSuits* trickSuit,
              CardSuits trumph,
              Card playedCard,
              std::vector<Card*>& playedCards,
              const Mode& mode);

    std::vector<Card> pickCards(int count);
    int calculateHand(const Mode& mode);
    bool checkPlayedCard(CardSuits trickSuit,
                         CardSuits trumph,
                         const Card& playedCard,
                         const std::vector<Card*>& playedCards,
                         const Mode& mode);
    void sortHand(const Mode& mode = Mode::BETL);

    Hand getHand();
    int getNumber();
    std::string getNick();
    std::string toString() const;
};

#endif