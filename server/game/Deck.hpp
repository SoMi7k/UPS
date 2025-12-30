#ifndef DECK_HPP
#define DECK_HPP

#include <vector>
#include "Card.hpp"

// Třída reprezentující balíček karet
class Deck {
private:
    static const int CARDS_NUMBER = 32;
    std::vector<Card*> cards;
    size_t cardIndex;
    
    // Privátní pomocné metody
    static std::vector<Card*> initializeDeck();
    
public:
    // Konstruktor
    Deck();
    
    // Destruktor (pokud potřebujete uvolnit SDL zdroje)
    ~Deck() = default;
    
    // Veřejné metody
    void shuffle(); // Zamíchá karty
    Card* dealCard(); // Vybere jednu kartu z balíčku
    bool hasNextCard() const; // Zjišťuje zda je v balíču karta
    const std::vector<Card*>& getCards() const { return cards; } // Vrátí balíček karet
};

#endif
