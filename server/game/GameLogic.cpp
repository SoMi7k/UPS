#include <algorithm>
#include <stdexcept>

#include "GameLogic.hpp"
GameLogic::GameLogic(int numPlayers) 
    : numPlayers(numPlayers), modeSet(false) {
}

// GETTERY - přístup k privátním členským proměnným
std::optional<CardSuits> GameLogic::getTrumph() const {
    return trumph;
}

std::vector<Card>& GameLogic::getTalon() {
    return talon;
}

Mode GameLogic::getMode() const {
    return mode;
}

bool GameLogic::isModeSet() const {
    return modeSet;
}

// SETTERY - nastavení privátních členských proměnných
void GameLogic::setTrumph(std::optional<CardSuits> newTrumph) {
    trumph = newTrumph;
}

void GameLogic::setMode(Mode newMode) {
    mode = newMode;
    modeSet = true;
}

// MOVE_TO_TALON - přesune kartu z ruky hráče do talonu
void GameLogic::moveToTalon(Card card, Player& player) {
    player.getHand().removeCard(card);
    talon.push_back(card);
}

// MOVE_FROM_TALON - přesune všechny karty z talonu hráči
void GameLogic::moveFromTalon(Player& player) {
    for (const Card& card : talon) {
        player.addCard(card);
    }
    talon.clear();
}

// FIND_TRUMPH - označí, které karty jsou trumfy
std::vector<bool> GameLogic::findTrumph(const std::map<int, Card>& cards, std::vector<bool> decisionList) {
    for (size_t i = 0; i < cards.size(); i++) {
        // Porovnáme barvu karty s naší trumfovou barvou
        if (cards.at(i).getSuit() != *trumph) {
            decisionList[i] = false;
        }
    }
    return decisionList;
}

// SAME_SUIT_CASE - najde výherce, když jsou všechny karty stejné barvy
int GameLogic::sameSuitCase(const std::map<int, Card>& cards) {
    int winnerIndex = 0;  // Začínáme s první kartou jako výhercem

    for (size_t i = 1; i < cards.size(); i++) {
        if (cards.at(i).getValue(mode) > cards.at(winnerIndex).getValue(mode)) {
            winnerIndex = i;
        }
    }
    return winnerIndex;
}

int GameLogic::notTrumphCase(const std::map<int, Card>& cards, CardSuits trickSuit) {
    int winnerIndex = 0;
    
    for (size_t i = 1; i < cards.size(); i++) {
        if (cards.at(i).getSuit() == trickSuit &&
            cards.at(i).getValue(mode) > cards.at(winnerIndex).getValue(mode)) {
            winnerIndex = i;
        }
    }
    return winnerIndex;
}

// TRICK_DECISION - vyhodnotí štych a vrátí index výherce
std::pair<int, Card> GameLogic::trickDecision(const std::map<int, Card>& cards, int startPlayerIndex) {
    CardSuits trickSuit = cards.at(startPlayerIndex).getSuit();
    std::vector<bool> decisionList(numPlayers, true);

    if (mode == Mode::HRA) {
        decisionList = findTrumph(cards, decisionList);
        
        // Spočítáme, kolik je trumfů
        int trumphCount = 0;
        for (bool isTrumph : decisionList) {
            if (isTrumph) trumphCount++;
        }
        
        // Pokud je jen jeden trumf, automaticky vyhrává
        if (trumphCount == 1) {
            for (size_t i = 0; i < decisionList.size(); i++) {
                if (decisionList[i]) {
                    return std::make_pair(i, cards.at(i));
                }
            }
        }
        
        // Pokud jsou dva trumfy, porovnáme je
        if (trumphCount == 2) {
            std::vector<int> trueIndices;
            for (size_t i = 0; i < decisionList.size(); i++) {
                if (decisionList[i]) {
                    trueIndices.push_back(i);
                }
            }
            
            if (cards.at(trueIndices[0]).getValue(mode) > cards.at(trueIndices[1]).getValue(mode)) {
                return std::make_pair(trueIndices[0], cards.at(trueIndices[0]));
            } else {
                return std::make_pair(trueIndices[1], cards.at(trueIndices[1]));
            }
        }
        
        // Pokud jsou tři trumfy, najdeme nejvyšší
        if (trumphCount == 3) {
            int winningIndex = sameSuitCase(cards);
            return std::make_pair(winningIndex, cards.at(winningIndex));
        }
    }
    
    // Označíme karty, které nejsou správné barvy
    for (size_t i = 0; i < cards.size(); i++) {
        if (i == 0) continue;  // První kartu přeskočíme
        if (cards.at(i).getSuit() != trickSuit) {
            decisionList[i] = false;
        }
    }
    
    // Najdeme výherce mezi kartami správné barvy
    int winningIndex = notTrumphCase(cards, trickSuit);
    return std::make_pair(winningIndex, cards.at(winningIndex));
}