#ifndef CARDS_HPP
#define CARDS_HPP

#include <string>
#include <vector>

// Enumy pro stavy hry
enum class State {
    ROZDANI_KARET = 0,
    LICITACE_TRUMF = 1,
    LICITACE_TALON = 2,
    LICITACE_HRA = 3,
    LICITACE_DOBRY_SPATNY = 4,
    LICITACE_BETL_DURCH = 5,
    HRA = 6,
    BETL = 7,
    DURCH = 8,
    END = 9
};

// Herní módy
enum class Mode {
    HRA = 1,
    BETL = 4,
    DURCH = 5
};

// Hodnoty karet
enum class CardRanks {
    A = 8,
    X = 7,
    K = 6,
    Q = 5,
    J = 4,
    IX = 3,
    VIII = 2,
    VII = 1,
};

// Barvy karet
enum class CardSuits {
    SRDCE,   // ♥
    KULE,    // ♦
    ZALUDY,  // ♣
    LISTY,   // ♠
};

// Třída reprezentující jednu kartu
class Card {
private:
    CardRanks rank;
    CardSuits suit;
    
public:
    // Konstruktor
    Card(CardRanks rank, CardSuits suit);
    
    // Gettery
    CardRanks getRank() const;

    CardSuits getSuit() const;
    
    // Operátory porovnání
    bool operator==(const Card& other) const;
    bool operator!=(const Card& other) const;
    
    // Metody pro práci s kartou
    std::string toString() const;
    int getValue(Mode gameMode) const;
};

// Pomocné funkce pro mapování
Card cardMapping(const std::string& inputStr);

// Pomocné funkce pro převod enum na string
std::string suitToString(CardSuits suit);
std::string rankToString(CardRanks rank);
std::string suitToSymbol(CardSuits suit);
std::string modeToString(Mode mode);

#endif // CARDS_HPP