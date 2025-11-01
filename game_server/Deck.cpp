// ============ Implementace třídy Deck ============
#include <algorithm>
#include <random>
#include <stdexcept>
#include "Card.hpp"
#include "Deck.hpp"

Deck::Deck() : cardIndex(0) {
    cards = initializeDeck();
    shuffle();
}

std::vector<Card> Deck::initializeDeck() {
    std::vector<Card> deck;
    deck.reserve(CARDS_NUMBER);

    // Všechny kombinace barev a hodnot
    for (int s = 0; s < 4; s++) {
        auto suit = static_cast<CardSuits>(s);
        for (int r = 1; r <= 8; r++) {
            auto rank = static_cast<CardRanks>(r);
            deck.emplace_back(rank, suit);
        }
    }

    return deck;
}

void Deck::shuffle() {
    cards = initializeDeck();

    // Použití moderního C++ random generátoru
    std::random_device rd;
    std::mt19937 gen(rd());
    std::shuffle(cards.begin(), cards.end(), gen);

    cardIndex = 0;
}

Card* Deck::dealCard() {
    if (hasNextCard()) {
        return &cards[cardIndex++];
    }
    return nullptr;
}

bool Deck::hasNextCard() const {
    return cardIndex < cards.size();
}

const std::vector<Card>& Deck::getCards() const {
    return cards;
}
