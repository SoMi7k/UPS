#include <algorithm>
#include <stdexcept>
#include <map>

#include "Hand.hpp"

#include <iostream>
#include <ostream>

#include "Deck.hpp"
#include "Card.hpp"

bool Hand::checkRightInput(const std::string& cardInput) {
    try {
        cardMapping(cardInput);
        return true;
    } catch (const std::invalid_argument&) {
        return false;
    }
}

void Hand::sort(const Mode& gameMode) {
    std::map<CardSuits, int> suitOrder = {
        {CardSuits::SRDCE, 0},
        {CardSuits::KULE, 1},
        {CardSuits::ZALUDY, 2},
        {CardSuits::LISTY, 3}
    };

    std::sort(cards.begin(), cards.end(),
        [&](const Card& a, const Card& b) {
            if (suitOrder[a.getSuit()] != suitOrder[b.getSuit()])
                return suitOrder[a.getSuit()] < suitOrder[b.getSuit()];
            return a.getValue(gameMode) > b.getValue(gameMode);
        });
}

bool Hand::findCardInHand(const Card& cardInput) const {
    for (const auto& card : cards) {
        if (card == cardInput)
            return true;
    }
    return false;
}

bool Hand::findCardByRank(CardRanks rank, const Mode& mode) const {
    for (const auto& card : cards) {
        if (card.getValue(mode) == static_cast<int>(rank))
            return true;
    }
    return false;
}

bool Hand::findCardBySuit(CardSuits suit) const {
    for (const auto& card : cards) {
        if (card.getSuit() == suit)
            return true;
    }
    return false;
}

void Hand::removeCard(const Card& cardToRemove) {
    cards.erase(std::remove(cards.begin(), cards.end(), cardToRemove), cards.end());
}

void Hand::addCard(const Card& cardToAdd) {
    //std::cout << "Card added " << cardToAdd.toString() << std::endl;
    cards.push_back(cardToAdd);
}

void Hand::addWonCard(const Card& card) {
    won_cards.push_back(card);
}

void Hand::calculateHand(const Mode& mode) {
    for (const auto& card : won_cards) {
        int value = card.getValue(mode);
        if (value == 7 || value == 8)
            points += 10;
    }
}

void Hand::removeHand() {
    if (!cards.empty()) {
        cards.clear();
    }
    if (!won_cards.empty()) {
        won_cards.clear();
    }
}

void Hand::showCards() {
    for (const auto& card : cards) {
        std::cout << "  - " << card.toString() << std::endl;
    }
}

