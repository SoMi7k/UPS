#ifndef PLAYER_CPP
#define PLAYER_CPP

#pragma once
#include <vector>
#include <string>
#include <map>
#include <optional>

#include "Hand.hpp"

class Player {
private:
    int number;
    Hand hand;
    std::string nick;
    std::string invalid_move;

public:
    explicit Player(int number, std::string nick);
    void removeHand();
    void addCard(const Card& card);
    void addWonCard(const Card& card);
    bool hasCardInHand() const;
    bool hasWon() const;

    std::vector<Card> pickCards(int count);
    int calculateHand(const Mode& mode);
    bool checkPlayedCard(CardSuits trickSuit,
                         std::optional<CardSuits>,
                         const Card& playedCard,
                         const std::map<int, Card>& playedCards,
                         const Mode& mode);
    void sortHand(const Mode& mode = Mode::BETL);

    Hand& getHand();
    int getNumber();
    std::string getNick();
    std::string toString() const;
    std::string getInvalidMove();
};

#endif