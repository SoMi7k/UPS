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

    bool checkRightInput(const std::string& cardInput); // Zjistí zda hráč zahrál správnou kartu
    void sort(const Mode& gameMode); // Seřadí karty podle barev a velikosti
    bool findCardInHand(const Card& cardInput) const; // Nalezne konkrétní kartu
    bool findCardByRank(CardRanks rank, const Mode& mode) const; // Nalezne kartu dle velikosti
    bool findCardBySuit(CardSuits suit) const; // Nalezne kartu dle barvy
    void removeCard(const Card& cardToRemove); // Odstraní kartu z ruky
    void addCard(const Card& cardToAdd); // Přidá kartu do ruky
    void addWonCard(const Card& card); // Přidá vítězné karty
    void calculateHand(const Mode& mode); // Spočítá ruku hráče
    void removeHand(); // Vymaže obě pole třídy Hand

    // Gettery
    std::vector<Card> getCards() const { return cards; }
    const std::vector<Card>& getWonCards() const { return won_cards; }
    int getPoints() const { return points; }
};

#endif