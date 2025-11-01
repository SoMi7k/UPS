#ifndef HAND_CPP
#define HAND_CPP

#pragma once
#include <vector>
#include <string>
#include "Card.hpp"

class Hand {
private:
    std::vector<Card> cards;
    std::vector<Card> won_cards;
    int points = 0;

public:
    Hand() = default;

    bool checkRightInput(const std::string& cardInput);
    void sort(const Mode& gameMode);
    bool findCardInHand(const Card& cardInput) const;
    bool findCardByRank(CardRanks rank, const Mode& mode) const;
    bool findCardBySuit(CardSuits suit) const;
    void removeCard(const Card& cardToRemove);
    void addCard(const Card& cardToAdd);
    void addWonCard(const Card& card);
    void calculateHand(const Mode& mode);
    void removeHand();
    const std::vector<Card>& getCards() const { return cards; }
    const std::vector<Card>& getWonCards() const { return won_cards; }
    int getPoints() const { return points; }
};

#endif