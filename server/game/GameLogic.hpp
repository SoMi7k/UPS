#ifndef GAME_LOGIC_HPP
#define GAME_LOGIC_HPP

#include <vector>
#include <utility>  // pro std::pair
#include <optional>

#include "Player.hpp"

// Třída GameLogic — logika celé hry
class GameLogic {
private:
    std::optional<CardSuits> trumph;           // Trumfová barva
    std::vector<Card> talon;    // Talon (karty stranou)
    int numPlayers;             // Počet hráčů
    Mode mode = Mode::HRA;                  // Herní mód (HRA, BETL, DURCH)
    bool modeSet;               // Zda byl mód nastaven

public:
    // Konstruktor
    GameLogic(int numPlayers);
    
    // Gettery
    std::optional<CardSuits> getTrumph() const;
    std::vector<Card>& getTalon();
    Mode getMode() const;
    bool isModeSet() const;
    
    // Settery
    void setTrumph(std::optional<CardSuits> newTrumph);
    void setMode(Mode newMode);
    
    // Metody pro práci s talonem
    void moveToTalon(Card card, Player& player);
    void moveFromTalon(Player& player);
    
    // Metody pro vyhodnocování štychů
    std::vector<bool> findTrumph(const std::map<int, Card>& cards, std::vector<bool> decisionList);
    int sameSuitCase(const std::map<int, Card>& cards);
    int notTrumphCase(const std::map<int, Card>& cards, CardSuits trickSuit);
    std::pair<int, Card> trickDecision(const std::map<int, Card>& cards, int startPlayerIndex);
};

#endif