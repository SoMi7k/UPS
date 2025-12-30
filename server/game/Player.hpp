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
    void removeHand(); // Vymaže pole třídy Hand
    void addCard(const Card& card); // Přidá kartu do ruky hráče
    void addWonCard(const Card& card); // Přidá vítěznou kartu
    bool hasCardInHand() const; // Má hráč kartu v ruce?

    std::vector<Card> pickCards(int count); // Vybere konkrétní počet karet
    int calculateHand(const Mode& mode); // Spočítá nahrané karty
    bool checkPlayedCard(CardSuits trickSuit,
                         std::optional<CardSuits>,
                         const Card& playedCard,
                         const std::map<int, Card>& playedCards,
                         const Mode& mode); // Zkontroluje správný vstup
    void sortHand(const Mode& mode = Mode::BETL); // Seřadí karty v ruce
    std::string toString() const;

    // GETTERY
    Hand& getHand();
    int getNumber();
    std::string getNick();
    std::string getInvalidMove();
};

#endif