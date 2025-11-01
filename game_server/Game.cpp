#include <utility>
#include <vector>
#include <memory>

#include "Game.hpp"

std::vector<int> ROZDAVANI_KARET = {7, 5, 5, 5};

Game::Game(int numPlayers)
    : numPlayers(numPlayers),
      gameLogic(numPlayers),
      state(State::ROZDANI_KARET),
      higherGame(false),
      higherPlayer(nullptr),
      trickSuitSet(false),
      trickWinnerSet(false),
      waitingForTrickEnd(false),
      talon(0) {

    players.resize(numPlayers, nullptr);
    playedCards.resize(numPlayers, nullptr);

    // Inicializujeme pole zahraných karet (všechny nullptr)
    playedCards.resize(numPlayers, nullptr);
}

// DESTRUKTOR - uvolníme dynamicky alokované hráče
Game::~Game() {
    for (Player* player : players) {
        delete player;  // Uvolníme paměť pro každého hráče
    }
}

// GETTERY
const std::vector<Player*>& Game::getPlayers() const {
    return players;
}

Player* Game::getPlayer(int index) const {
    if (index < 0 || index >= static_cast<int>(players.size())) {
        return nullptr;
    }
    return players[index];
}

void Game::initPlayer(int number, std::string nick) {
    if (number < 0 || number >= static_cast<int>(players.size())) {
        std::cerr << "Chybný index hráče: " << number << std::endl;
        return;
    }

    players[number] = new Player(number, std::move(nick));
}

void Game::defineLicitator(int number) {
    licitator = players[number];
    activePlayer = licitator;
    startingPlayerIndex = licitator->getNumber();
}

Player* Game::getLicitator() const {
    return licitator;
}

State Game::getState() const {
    return state;
}

Player* Game::getActivePlayer() const {
    return activePlayer;
}

const std::vector<Card*>& Game::getPlayedCards() const {
    return playedCards;
}

bool Game::isWaitingForTrickEnd() const {
    return waitingForTrickEnd;
}

int Game::getTrickWinner() const {
    return trickWinner;
}

const std::vector<Card*>& Game::getTrickCards() const {
    return trickCards;
}

std::string Game::getResultStr() const {
    return resultStr;
}

GameLogic& Game::getGameLogic() {
    return gameLogic;
}

// DEAL_CARDS - rozdá karty podle pravidel
// V Pythonu: def deal_cards(self):
void Game::dealCards() {
    // První kolo - 7 karet licitátorovi
    for (int i = 0; i < ROZDAVANI_KARET[0]; i++) {
        Card* card = deck.dealCard();
        if (card) licitator->addCard(*card);
    }

    // Druhé kolo - 5 karet ostatním
    for (int i = 0; i < ROZDAVANI_KARET[1]; i++) {
        for (Player* player : players) {
            if (player != licitator) {
                Card* card = deck.dealCard();
                if (card) player->addCard(*card);
            }
        }
    }

    // Třetí kolo - 5 karet licitátorovi
    for (int i = 0; i < ROZDAVANI_KARET[2]; i++) {
        Card* card = deck.dealCard();
        if (card) licitator->addCard(*card);
    }

    // Čtvrté kolo - 5 karet ostatním
    for (int i = 0; i < ROZDAVANI_KARET[3]; i++) {
        for (Player* player : players) {
            if (player != licitator) {
                Card* card = deck.dealCard();
                if (card) player->addCard(*card);
            }
        }
    }

    // Seřadíme ruce všem hráčům
    for (Player* player : players) {
        player->sortHand();
    }
}

// RESET_GAME - resetuje hru na začátek
// V Pythonu: def reset_game(self):
void Game::resetGame() {
    for (Player* player : players) {
        player->removeHand();
    }
    deck.shuffle();
}

// NEXT_PLAYER - přepne na dalšího hráče
// V Pythonu: def next_player(self):
void Game::nextPlayer() {
    int actualPlayerIndex = activePlayer->getNumber();
    activePlayer = players[(actualPlayerIndex + 1) % numPlayers];
}

// GAME_STATE_1 - nastavení trumfové barvy
// V Pythonu: def game_state_1(self, card: Card):
void Game::gameState1(Card card) {
    gameLogic.setTrumph(card.getSuit());  // Nastavíme trumf podle karty
    state = State::LICITACE_TALON;        // Přejdeme na další stav
}

// GAME_STATE_2 - dávání karet do talonu
// V Pythonu: def game_state_2(self, card: Card):
void Game::gameState2(Card card) {
    gameLogic.moveToTalon(card, *activePlayer);  // * = dereferujeme ukazatel

    if (gameLogic.getTalon().size() == 2) {
        if (higherGame && gameLogic.getMode() == Mode::BETL &&
            higherPlayer->getNumber() != numPlayers - 1) {
            talon = 0;
            state = State::LICITACE_DOBRY_SPATNY;
            nextPlayer();
            return;
        }
        if (higherGame && gameLogic.getMode() == Mode::BETL) {
            state = State::BETL;
            return;
        }
        if (higherGame && gameLogic.getMode() == Mode::DURCH) {
            state = State::DURCH;
            return;
        }
        state = State::LICITACE_HRA;
        talon = 0;
    }
}

// GAME_STATE_3 - volba herního módu
// V Pythonu: def game_state_3(self, label: str):
void Game::gameState3(const std::string& label) {
    if (gameLogic.isModeSet()) {
        return;
    }

    if (label == "HRA") {
        gameLogic.setMode(Mode::HRA);
    } else if (label == "BETL") {
        higher(activePlayer, Mode::BETL);
    } else {
        higher(activePlayer, Mode::DURCH);
        state = State::DURCH;
        return;
    }

    state = State::LICITACE_DOBRY_SPATNY;
    nextPlayer();
}

// GAME_STATE_4 - reakce na "Dobrý"/"Špatný"
// V Pythonu: def game_state_4(self, label: str):
void Game::gameState4(const std::string& label) {
    if (label == "Špatný" && !higherGame) {
        higherGame = true;
        higherPlayer = activePlayer;
        state = State::LICITACE_BETL_DURCH;
        return;
    }

    if (label == "Špatný" && higherGame) {
        higherGame = true;
        higher(activePlayer, Mode::DURCH);
        return;
    }

    if (label == "Dobrý" && higherGame) {
        state = State::BETL;
        activePlayer = players[higherPlayer->getNumber()];
        return;
    }

    if (label == "Dobrý" && activePlayer->getNumber() == numPlayers - 1) {
        chooseModeState();
    }

    nextPlayer();
}

// GAME_STATE_5 - volba mezi BETL a DURCH
// V Pythonu: def game_state_5(self, label: str):
void Game::gameState5(const std::string& label) {
    if (label == "DURCH") {
        higher(activePlayer, Mode::DURCH);
        return;
    }

    if (label == "BETL" && higherPlayer->getNumber() != numPlayers - 1) {
        higher(activePlayer, Mode::BETL);
    } else {
        higher(activePlayer, Mode::BETL);
        higherPlayer = activePlayer;
    }
}

// CHECK_STYCH - kontrola konce štychu
// V Pythonu: def check_stych(self) -> bool:
bool Game::checkStych() {
    if (waitingForTrickEnd) {
        // Přidáme karty výherci
        for (Card* wonCard : trickCards) {
            players[trickWinner]->addWonCard(*wonCard);
        }

        // Resetujeme pro další štych
        std::fill(playedCards.begin(), playedCards.end(), nullptr);
        trickSuitSet = false;
        startingPlayerIndex = trickWinner;
        trickWinnerSet = false;
        trickCards.clear();
        waitingForTrickEnd = false;
        activePlayer = players[startingPlayerIndex];
        return true;
    }
    return false;
}

// GAME_STATE_6 - normální hra (HRA)
// V Pythonu: def game_state_6(self, card: Card) -> bool:
bool Game::gameState6(Card card) {
    if (trickSuitSet) {
        if (!activePlayer->checkPlayedCard(trickSuit, gameLogic.getTrumph(),
                                           card, playedCards, gameLogic.getMode())) {
            return false;
        }
    } else {
        trickSuit = card.getSuit();
        trickSuitSet = true;
    }

    playedCards[activePlayer->getNumber()] = &card;
    activePlayer->getHand().removeCard(card);

    // Zkontrolujeme, zda je štych kompletní
    bool allCardsPlayed = true;
    for (Card* c : playedCards) {
        if (c == nullptr) {
            allCardsPlayed = false;
            break;
        }
    }

    if (allCardsPlayed) {
        auto result = gameLogic.trickDecision(playedCards, startingPlayerIndex);
        trickWinner = result.first;
        trickWinnerSet = true;
        trickCards = playedCards;
        waitingForTrickEnd = true;
    }

    // Zkontrolujeme konec hry
    bool allHandsEmpty = true;
    for (Player* player : players) {
        if (player->hasCardInHand()) {
            allHandsEmpty = false;
            break;
        }
    }

    if (allHandsEmpty) {
        resultStr = gameResult(nullptr);
        state = State::END;
    }

    nextPlayer();
    return true;
}

// GAME_STATE_7 - BETL nebo DURCH
// V Pythonu: def game_state_7(self, card: Card):
bool Game::gameState7(Card card) {
    if (trickSuitSet) {
        if (!activePlayer->checkPlayedCard(trickSuit, gameLogic.getTrumph(),
                                           card, playedCards, gameLogic.getMode())) {
            return false;
        }
    } else {
        trickSuit = card.getSuit();
        trickSuitSet = true;
    }

    playedCards[activePlayer->getNumber()] = &card;
    activePlayer->getHand().removeCard(card);

    bool allCardsPlayed = true;
    for (Card* c : playedCards) {
        if (c == nullptr) {
            allCardsPlayed = false;
            break;
        }
    }

    if (allCardsPlayed) {
        auto result = gameLogic.trickDecision(playedCards, startingPlayerIndex);
        int winnerIndex = result.first;
        trickWinner = winnerIndex;
        trickWinnerSet = true;
        trickCards = playedCards;
        waitingForTrickEnd = true;

        // Kontrola prohry v BETL/DURCH
        if (gameLogic.getMode() == Mode::BETL) {
            if (winnerIndex == licitator->getNumber()) {
                bool loss = false;
                resultStr = gameResult(&loss);
                state = State::END;
                return true;
            }
        } else {
            if (winnerIndex != licitator->getNumber()) {
                bool loss = false;
                resultStr = gameResult(&loss);
                state = State::END;
                return true;
            }
        }

        for (Card* wonCard : playedCards) {
            players[winnerIndex]->addWonCard(*wonCard);
        }
    }

    bool allHandsEmpty = true;
    for (Player* player : players) {
        if (player->hasCardInHand()) {
            allHandsEmpty = false;
            break;
        }
    }

    if (allHandsEmpty) {
        bool win = true;
        resultStr = gameResult(&win);
        state = State::END;
    }

    nextPlayer();
    return true;
}

// GAME_STATE_8 - konec hry
// V Pythonu: def game_state_8(self, label: str) -> bool:
bool Game::gameState8(const std::string& label) {
    return label == "ANO";
}

// CHOOSE_MODE_STATE - výběr herního módu
// V Pythonu: def choose_mode_state(self):
void Game::chooseModeState() {
    switch (gameLogic.getMode()) {
        case Mode::HRA:
            state = State::HRA;
            break;
        case Mode::BETL:
            state = State::BETL;
            break;
        case Mode::DURCH:
            state = State::DURCH;
            break;
        default:
            state = State::HRA;
            gameLogic.setMode(Mode::HRA);
            break;
    }
}

// HIGHER - hlášení vyšší hry
// V Pythonu: def higher(self, player: Player, mode: Mode):
void Game::higher(Player* player, Mode mode) {
    gameLogic.setMode(mode);
    // V C++ nemůžeme nastavit nullptr na CardSuits enum
    higherPlayer = player;

    if (player != licitator) {
        licitator = player;
        gameLogic.moveFromTalon(*player);
        state = State::LICITACE_TALON;
    }
}

// GAME_RESULT - vyhodnocení výsledku hry
// V Pythonu: def game_result(self, res: None|bool) -> str:
std::string Game::gameResult(bool* result) {
    if (result != nullptr) {
        if (*result) {
            return "Vyhrál Hráč #" + std::to_string(licitator->getNumber()) + " jako licitátor!";
        } else {
            return "Prohrál Hráč #" + std::to_string(licitator->getNumber()) + " jako licitátor!";
        }
    }

    int playersPoints = 0;
    int licitatorPoints = licitator->calculateHand(gameLogic.getMode());

    for (Player* player : players) {
        if (player != licitator) {
            playersPoints += player->calculateHand(gameLogic.getMode());
        }
    }

    if (licitatorPoints > playersPoints) {
        return "Vyhrál Hráč #" + std::to_string(licitator->getNumber()) +
               " jako licitátor v poměru " + std::to_string(licitatorPoints) +
               " : " + std::to_string(playersPoints);
    } else if (licitatorPoints == playersPoints) {
        return "Remíza v poměru Hráč #" + std::to_string(licitator->getNumber()) +
               " jako licitátor " + std::to_string(licitatorPoints) +
               " : Hráči proti " + std::to_string(playersPoints);
    } else {
        return "Prohrál Hráč #" + std::to_string(licitator->getNumber()) +
               " jako licitátor v poměru " + std::to_string(licitatorPoints) +
               " : " + std::to_string(playersPoints);
    }
}

void Game::gameHandler(Card &card, std::string &label) {
    while (true) {
        State oldState = state;
        switch (static_cast<int>(state)) {
            case 1: gameState1(card);
            case 2: gameState2(card);
            case 3: gameState3(label);
            case 4: gameState4(label);
            case 5: gameState5(label);
            case 6: gameState6(card);
            case 7: gameState7(card);
            case 8: gameState8(label);
            default: std::cout << "ERROR: Unknown state" << std::endl; break;
        }
        if (oldState != state) {
            stateChanged = true;
        }
    }
}
