#ifndef GAME_LOGIC_HPP
#define GAME_LOGIC_HPP

#include <vector>
#include <utility>  // pro std::pair
#include "Player.hpp"

// Třída GameLogic — logika celé hry
class GameLogic {
private:
    CardSuits trumph;           // Trumfová barva
    std::vector<Card> talon;    // Talon (karty stranou)
    int numPlayers;             // Počet hráčů
    Mode mode;                  // Herní mód (HRA, BETL, DURCH)
    bool modeSet;               // Zda byl mód nastaven

public:
    // Konstruktor
    GameLogic(int numPlayers);
    
    // Gettery
    CardSuits getTrumph() const;
    const std::vector<Card>& getTalon() const;
    Mode getMode() const;
    bool isModeSet() const;
    
    // Settery
    void setTrumph(CardSuits newTrumph);
    void setMode(Mode newMode);
    
    // Metody pro práci s talonem
    void moveToTalon(Card card, Player& player);
    void moveFromTalon(Player& player);
    
    // Metody pro vyhodnocování štychů
    std::vector<bool> findTrumph(const std::vector<Card*>& cards, std::vector<bool> decisionList);
    int sameSuitCase(const std::vector<Card*>& cards);
    int notTrumphCase(const std::vector<Card*>& cards, CardSuits trickSuit);
    std::pair<int, Card*> trickDecision(const std::vector<Card*>& cards, int startPlayerIndex);
};

#endif