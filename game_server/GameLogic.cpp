#include <algorithm>
#include <stdexcept>

#include "GameLogic.hpp"

// ============ Implementace třídy GameLogic ============

// KONSTRUKTOR
// V Pythonu: def __init__(self, num_players: int):
GameLogic::GameLogic(int numPlayers) 
    : numPlayers(numPlayers), modeSet(false) {
    // trumph se nastaví později
    // talon je prázdný vector
    // mode se nastaví později
}

// GETTERY - přístup k privátním členským proměnným
CardSuits GameLogic::getTrumph() const {
    return trumph;  // Vrací naši členskou proměnnou trumph
}

const std::vector<Card>& GameLogic::getTalon() const {
    return talon;  // Vrací referenci na náš talon
}

Mode GameLogic::getMode() const {
    return mode;  // Vrací náš herní mód
}

bool GameLogic::isModeSet() const {
    return modeSet;  // Vrací, zda byl mód nastaven
}

// SETTERY - nastavení privátních členských proměnných
void GameLogic::setTrumph(CardSuits newTrumph) {
    trumph = newTrumph;  // Nastavíme naši členskou proměnnou
}

void GameLogic::setMode(Mode newMode) {
    mode = newMode;      // Nastavíme náš herní mód
    modeSet = true;      // Označíme, že mód je nastaven
}

// MOVE_TO_TALON - přesune kartu z ruky hráče do talonu
// V Pythonu: def move_to_talon(self, card: Card, player: Player):
void GameLogic::moveToTalon(Card card, Player& player) {
    // player je reference (&), takže pracujeme přímo s původním objektem
    player.getHand().removeCard(card);  // Odebereme kartu z ruky hráče
    talon.push_back(card);              // Přidáme kartu do našeho talonu
}

// MOVE_FROM_TALON - přesune všechny karty z talonu hráči
// V Pythonu: def move_from_talon(self, player: Player):
void GameLogic::moveFromTalon(Player& player) {
    // Projdeme všechny karty v našem talonu
    for (const Card& card : talon) {
        player.addCard(card);  // Přidáme kartu hráči
    }
    talon.clear();  // Vyprázdníme náš talon
}

// FIND_TRUMPH - označí, které karty jsou trumfy
// V Pythonu: def find_trumph(self, cards: list[Card], decision_list: list[bool]) -> list[bool]:
std::vector<bool> GameLogic::findTrumph(const std::vector<Card*>& cards, std::vector<bool> decisionList) {
    // cards jsou ukazatele na karty
    // decisionList je kopie (předáno hodnotou, ne referencí)
    
    for (size_t i = 0; i < cards.size(); i++) {
        // Porovnáme barvu karty s naší trumfovou barvou
        if (cards[i]->getSuit() != trumph) {
            decisionList[i] = false;  // Není trumf
        }
    }
    return decisionList;
}

// SAME_SUIT_CASE - najde výherce, když jsou všechny karty stejné barvy
// V Pythonu: def same_suit_case(self, cards: list[Card]) -> int:
int GameLogic::sameSuitCase(const std::vector<Card*>& cards) {
    int winnerIndex = 0;  // Začínáme s první kartou jako výhercem
    
    // Projdeme všechny ostatní karty
    for (size_t i = 1; i < cards.size(); i++) {
        // Porovnáme hodnotu aktuální karty s výherní kartou
        // cards[i] je ukazatel, takže použijeme ->
        if (cards[i]->getValue(mode) > cards[winnerIndex]->getValue(mode)) {
            winnerIndex = i;  // Našli jsme lepší kartu
        }
    }
    return winnerIndex;
}

// NOT_TRUMPH_CASE - najde výherce, když nikdo nezahrál trumf
// V Pythonu: def not_trumph_case(self, cards: list[Card], trick_suit: CardSuits) -> int:
int GameLogic::notTrumphCase(const std::vector<Card*>& cards, CardSuits trickSuit) {
    int winnerIndex = 0;
    
    for (size_t i = 1; i < cards.size(); i++) {
        // Karta musí být správné barvy A mít vyšší hodnotu
        if (cards[i]->getSuit() == trickSuit && 
            cards[i]->getValue(mode) > cards[winnerIndex]->getValue(mode)) {
            winnerIndex = i;
        }
    }
    return winnerIndex;
}

// TRICK_DECISION - vyhodnotí štych a vrátí index výherce
// V Pythonu: def trick_decision(self, cards: list[Card], start_player_index: int) -> tuple[int, Card]:
std::pair<int, Card*> GameLogic::trickDecision(const std::vector<Card*>& cards, int startPlayerIndex) {
    // Barva první karty určuje barvu štychu
    CardSuits trickSuit = cards[startPlayerIndex]->getSuit();
    
    // Vytvoříme decision list (všechny true na začátku)
    std::vector<bool> decisionList(numPlayers, true);
    
    // Pokud hrajeme normální hru (HRA)
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
                    return std::make_pair(i, cards[i]);
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
            
            if (cards[trueIndices[0]]->getValue(mode) > cards[trueIndices[1]]->getValue(mode)) {
                return std::make_pair(trueIndices[0], cards[trueIndices[0]]);
            } else {
                return std::make_pair(trueIndices[1], cards[trueIndices[1]]);
            }
        }
        
        // Pokud jsou tři trumfy, najdeme nejvyšší
        if (trumphCount == 3) {
            int winningIndex = sameSuitCase(cards);
            return std::make_pair(winningIndex, cards[winningIndex]);
        }
    }
    
    // Označíme karty, které nejsou správné barvy
    for (size_t i = 0; i < cards.size(); i++) {
        if (i == 0) continue;  // První kartu přeskočíme
        if (cards[i]->getSuit() != trickSuit) {
            decisionList[i] = false;
        }
    }
    
    // Najdeme výherce mezi kartami správné barvy
    int winningIndex = notTrumphCase(cards, trickSuit);
    return std::make_pair(winningIndex, cards[winningIndex]);
}