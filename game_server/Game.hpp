#ifndef GAME_HPP
#define GAME_HPP

#include "GameLogic.hpp"
#include "Deck.hpp"

class Game {
private:
    int numPlayers;                      // Počet hráčů
    std::vector<Player*> players;        // Pole hráčů
    Player* licitator;                   // Aktuální licitátor
    Deck deck;                           // Balíček karet
    GameLogic gameLogic;                 // Herní logika
    std::vector<Card*> playedCards;      // Karty zahrané v aktuálním štychu
    State state;                         // Aktuální stav hry
    bool higherGame;                     // Zda někdo hlásí vyšší hru
    Player* higherPlayer;                // Hráč, který hlásí vyšší hru
    int startingPlayerIndex;             // Index hráče, který začíná štych
    Player* activePlayer;                // Aktivní hráč na tahu
    CardSuits trickSuit;                 // Barva štychu (první zahraná karta)
    bool trickSuitSet;                   // Zda byla nastavena barva štychu
    int trickWinner;                     // Index výherce štychu
    bool trickWinnerSet;                 // Zda byl nastaven výherce
    std::vector<Card*> trickCards;       // Karty ze štychu pro zobrazení
    bool waitingForTrickEnd;             // Čeká se na konec štychu
    std::string resultStr;               // Výsledek hry
    int talon;                           // Počitadlo pro talon
    bool stateChanged;                   // Příznak pro změnu stavu hry

public:
    // Konstruktor a destruktor
    explicit Game(int numPlayers = 3);
    ~Game();
    
    // Gettery
    const std::vector<Player*>& getPlayers() const;
    Player* getPlayer(int index) const;
    void initPlayer(int number, std::string nick);
    void defineLicitator(int number);
    Player* getLicitator() const;
    State getState() const;
    Player* getActivePlayer() const;
    const std::vector<Card*>& getPlayedCards() const;
    bool isWaitingForTrickEnd() const;
    int getTrickWinner() const;
    const std::vector<Card*>& getTrickCards() const;
    std::string getResultStr() const;
    GameLogic& getGameLogic();
    
    // Hlavní herní metody
    void dealCards();
    void resetGame();
    void nextPlayer();
    
    // Metody pro jednotlivé stavy hry
    void gameState1(Card card);                    // LICITACE_TRUMF
    void gameState2(Card card);                    // LICITACE_TALON
    void gameState3(const std::string& label);     // LICITACE_HRA
    void gameState4(const std::string& label);     // LICITACE_DOBRY_SPATNY
    void gameState5(const std::string& label);     // LICITACE_BETL_DURCH
    bool gameState6(Card card);                    // HRA
    bool gameState7(Card card);                    // BETL/DURCH
    bool gameState8(const std::string& label);     // END

    void gameHandler(Card &card, std::string &label);
    
    // Pomocné metody
    bool checkStych();
    void chooseModeState();
    void higher(Player* player, Mode mode);
    std::string gameResult(bool* result);
};

#endif