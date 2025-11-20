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
    void shuffle();
    Card* dealCard();
    bool hasNextCard() const;
    void addCard(Card* card) { cards.push_back(card); }
    const std::vector<Card*>& getCards() const { return cards; }
    
    // Getter pro testování
    //const std::vector<Card> getCards() const;
};

#endif
