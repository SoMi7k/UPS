#include <random>
#include <stdexcept>
#include <sstream>
#include <map>

#include "Card.hpp"

// Globální mapy pro mapování stringů na enumy
static const std::map<std::string, CardSuits> SUITE_MAP = {
    {"♥", CardSuits::SRDCE},
    {"♦", CardSuits::KULE},
    {"♣", CardSuits::ZALUDY},
    {"♠", CardSuits::LISTY}
};

static const std::map<std::string, CardRanks> RANK_MAP = {
    {"a", CardRanks::A},
    {"k", CardRanks::K},
    {"q", CardRanks::Q},
    {"j", CardRanks::J},
    {"10", CardRanks::X},
    {"9", CardRanks::IX},
    {"8", CardRanks::VIII},
    {"7", CardRanks::VII}
};

// ============ Implementace třídy Card ============
// V Pythonu: def __init__(self, rank, suit):
// V C++: používáme inicializační seznam (: rank(rank), suit(suit))
// Je to efektivnější než přiřazení v těle konstruktoru
Card::Card(CardRanks rank, CardSuits suit) : rank(rank), suit(suit) {
    // Tělo konstruktoru je prázdné, protože inicializační seznam už vše udělal
    // Ale kdybyste potřebovali další logiku, napíšete ji sem
}

// GETTER pro rank
// V Pythonu: self.rank
// V C++: stačí napsat jen "rank" (bez this->), kompilátor ví, že jde o členskou proměnnou
CardRanks Card::getRank() const {
    return rank;
}

// GETTER pro suit
CardSuits Card::getSuit() const {
    return suit;
}

// OPERÁTOR == pro porovnání dvou karet
// V Pythonu: def __eq__(self, other):
// V C++: bool operator==(const Card& other)
// "other" je druhá karta, se kterou porovnáváme
bool Card::operator==(const Card& other) const {
    // Porovnáváme NAŠE rank a suit (this->rank, this->suit)
    // s rank a suit DRUHÉ karty (other.rank, other.suit)
    return rank == other.rank && suit == other.suit;

    // Ekvivalent s explicitním this:
    // return this->rank == other.rank && this->suit == other.suit;
}

// OPERÁTOR != (negace ==)
bool Card::operator!=(const Card& other) const {
    // *this znamená "tento objekt" (celá aktuální karta)
    return !(*this == other);  // Zavolá operátor == a neguje výsledek
}

// METODA toString - vrací textovou reprezentaci karty
// V Pythonu: def __str__(self):

std::string Card::toString() const {
    // Přistupujeme k členským proměnným rank a suit
    // a používáme pomocné funkce pro převod na string
    return rankToString(rank) + " " + suitToSymbol(suit);
    // např. "A ♥" pro eso srdcové
}

// METODA getValue - vrací číselnou hodnotu karty podle herního módu
// V Pythonu: def get_value(self, game_mode):
int Card::getValue(Mode gameMode) const {
    if (gameMode == Mode::HRA) {
        return static_cast<int>(rank);
    }

    if (gameMode == Mode::BETL || gameMode == Mode::DURCH) {
        // Pro betl a durch má desítka jinou hodnotu
        static const std::map<CardRanks, int> betlOrder = {
            {CardRanks::A, 8},
            {CardRanks::K, 7},
            {CardRanks::Q, 6},
            {CardRanks::J, 5},
            {CardRanks::X, 4},
            {CardRanks::IX, 3},
            {CardRanks::VIII, 2},
            {CardRanks::VII, 1}
        };
        // Hledáme hodnotu pro NAŠI kartu (podle rank)
        return betlOrder.at(rank);
    }

    // Neplatný mód - vyhodíme výjimku
    throw std::invalid_argument("Neplatný herní mód");
}

// ============ Pomocné funkce ============

Card cardMapping(const std::string& inputStr) {
    std::istringstream iss(inputStr);
    std::string first, second;
    iss >> first >> second;

    // Zkusíme první kombinaci (barva hodnost)
    if (SUITE_MAP.find(first) != SUITE_MAP.end() &&
        RANK_MAP.find(second) != RANK_MAP.end()) {
        return {RANK_MAP.at(second), SUITE_MAP.at(first)};
        }

    // Zkusíme druhou kombinaci (hodnost barva)
    if (RANK_MAP.find(first) != RANK_MAP.end() &&
        SUITE_MAP.find(second) != SUITE_MAP.end()) {
        return {RANK_MAP.at(first), SUITE_MAP.at(second)};
        }

    throw std::invalid_argument("Neplatný formát karty");
}

std::string suitToString(CardSuits suit) {
    switch (suit) {
        case CardSuits::SRDCE: return "SRDCE";
        case CardSuits::KULE: return "KULE";
        case CardSuits::ZALUDY: return "ZALUDY";
        case CardSuits::LISTY: return "LISTY";
        default: return "UNKNOWN";
    }
}

std::string rankToString(CardRanks rank) {
    switch (rank) {
        case CardRanks::A: return "a";
        case CardRanks::X: return "10";
        case CardRanks::K: return "k";
        case CardRanks::Q: return "q";
        case CardRanks::J: return "j";
        case CardRanks::IX: return "9";
        case CardRanks::VIII: return "8";
        case CardRanks::VII: return "7";
        default: return "UNKNOWN";
    }
}

std::string suitToSymbol(CardSuits suit) {
    switch (suit) {
        case CardSuits::SRDCE: return "♥";
        case CardSuits::KULE: return "♦";
        case CardSuits::ZALUDY: return "♣";
        case CardSuits::LISTY: return "♠";
        default: return "?";
    }
}

std::string modeToString(Mode mode) {
    switch (mode) {
        case Mode::HRA:   return "HRA";
        case Mode::BETL:  return "BETL";
        case Mode::DURCH: return "DURCH";
        default:          return "UNKNOWN";
    }
}