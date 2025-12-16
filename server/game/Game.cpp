#include <utility>
#include <vector>
#include <memory>
#include <iostream>

#include "Game.hpp"

std::vector<int> ROZDAVANI_KARET = {7, 5};

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

void Game::resetTrick(int playerNumber) {
    clearPlayedCards();
    Player* player = getPlayer(playerNumber);
    activePlayer = player;
    waitingForTrickEnd = false;
    trickWinnerSet = false;
    trickSuitSet = false;
}

State Game::getState() const {
    return state;
}

Player* Game::getActivePlayer() const {
    return activePlayer;
}

const std::map<int, Card>& Game::getPlayedCards() const {
    return playedCards;
}

bool Game::isWaitingForTrickEnd() const {
    return waitingForTrickEnd;
}

int Game::getTrickWinner() const {
    return trickWinner;
}

std::pair<int, int> Game::getResult() const {
    return result;
}

GameLogic& Game::getGameLogic() {
    return gameLogic;
}

void Game::clearPlayedCards() {
    if (!playedCards.empty()) {
        playedCards.clear();
    }
}

// DEAL_CARDS - rozdá karty podle pravidel
// V Pythonu: def deal_cards(self):
void Game::dealCards() {
    // První kolo - 12 karet licitátorovi
    for (int i = 0; i < ROZDAVANI_KARET[0]; i++) {
        Card *card = deck.dealCard();
        licitator->addCard(*card);
    }

    // Druhé kolo - 10 karet ostatním
    for (int i = 0; i < ROZDAVANI_KARET[1]; i++) {
        for (Player* player : players) {
            if (player != licitator) {
                Card *card = deck.dealCard();
                player->addCard(*card);
            }
        }
    }

    // Seřadíme ruce všem hráčům
    for (Player* player : players) {
        player->sortHand();
    }

    state = State::LICITACE_TRUMF;
}

// NEXT_PLAYER - přepne na dalšího hráče
void Game::nextPlayer() {
    int actualPlayerIndex = activePlayer->getNumber();
    activePlayer = players[(actualPlayerIndex + 1) % numPlayers];
}

// GAME_STATE_1 - nastavení trumfové barvy
void Game::gameState1(Card card) {
    std::cout << "Trumf: " << card.toString() << std::endl;
    gameLogic.setTrumph(card.getSuit());  // Nastavíme trumf podle karty
    state = State::LICITACE_TALON;        // Přejdeme na další stav
}

// GAME_STATE_2 - dávání karet do talonu
void Game::gameState2(Card card) {
    gameLogic.moveToTalon(card, *activePlayer);
    std::cout << "Odhazuji kartu " << card.toString() << std::endl;

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

    gameStarted = 1;
    std::cout << "Zvolena hra. Začíná licitace.\n";
}

// GAME_STATE_4 - reakce na "Dobrý"/"Špatný"
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
        activePlayer = players[licitator->getNumber()];
    }

    std::cout << "Zahlášeno " << label << std::endl;
}

// GAME_STATE_5 - volba mezi BETL a DURCH
void Game::gameState5(const std::string& label) {
    if (label == "DURCH") {
        higher(activePlayer, Mode::DURCH);
        return;
    }

    if (label == "BETL" && higherPlayer->getNumber() != numPlayers - 1) {
        higher(activePlayer, Mode::BETL);
        nextPlayer();
    } else {
        higher(activePlayer, Mode::BETL);
        higherPlayer = activePlayer;
    }

    std::cout << "Změna hry " << label << std::endl;
}

// CHOOSE_MODE_STATE - výběr herního módu
void Game::chooseModeState() {
    switch (gameLogic.getMode()) {
        case Mode::HRA:
            state = State::HRA;
            break;
        case Mode::BETL:
            state = State::BETL;
            std::cout << "Trumph vynulován!" << std::endl;
            gameLogic.setTrumph(std::nullopt);
            break;
        case Mode::DURCH:
            state = State::DURCH;
            std::cout << "Trumph vynulován!" << std::endl;
            gameLogic.setTrumph(std::nullopt);
            break;
        default:
            state = State::HRA;
            gameLogic.setMode(Mode::HRA);
            break;
    }
}

// HIGHER - hlášení vyšší hry
void Game::higher(Player* player, Mode mode) {
    gameLogic.setMode(mode);
    higherPlayer = player;

    if (player != licitator) {
        licitator = player;
        gameLogic.moveFromTalon(*player);
        state = State::LICITACE_TALON;
    }
}

// GAME_STATE_6 - normální hra (HRA)
bool Game::gameState6(Card card) {
    if (trickSuitSet) {
        std::cout << "Kontroluji zahranou kartu." << std::endl;
        if (!activePlayer->checkPlayedCard(trickSuit, gameLogic.getTrumph(), card, playedCards, gameLogic.getMode())) {
            return false;
        }
    } else {
        trickSuit = card.getSuit();
        trickSuitSet = true;
        std::cout << "Barvu štychu: " << suitToString(trickSuit) << std::endl;
    }

    std::cout << "Přidávám kartu " << card.toString() << std::endl;
    playedCards.insert_or_assign(activePlayer->getNumber(), card);
    activePlayer->getHand().removeCard(card);

    // Zkontrolujeme, zda je štych kompletní
    bool allCardsPlayed = false;
    if (static_cast<int>(playedCards.size()) == numPlayers) {
        allCardsPlayed = true;
    }

    if (allCardsPlayed) {
        std::cout << "Štych je kompletní, vyhodnocuji..." << std::endl;
        auto [fst, snd] = gameLogic.trickDecision(playedCards, startingPlayerIndex);
        std::cout << "Vítěz je #" << fst << " s vítěznou kartou " << snd.toString() << std::endl;
        trickWinner = fst;
        trickWinnerSet = true;
        waitingForTrickEnd = true;
        for (auto player_card : playedCards) {
            players[trickWinner]->addWonCard(player_card.second);
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
        std::cout << "Hráči již nemají karty, nastává konec hry" << std::endl;
        result = gameResult(nullptr);
        state = State::END;
        return true;
    }

    if (!waitingForTrickEnd) {
        nextPlayer();
    }
    return true;
}

// GAME_STATE_7 - BETL nebo DURCH
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

    playedCards.insert_or_assign(activePlayer->getNumber(), card);
    activePlayer->getHand().removeCard(card);

    // Zkontrolujeme, zda je štych kompletní
    bool allCardsPlayed = false;
    if (static_cast<int>(playedCards.size()) == numPlayers) {
        allCardsPlayed = true;
    }

    if (allCardsPlayed) {
        std::cout << "Štych je kompletní, vyhodnocuji..." << std::endl;
        auto [fst, snd] = gameLogic.trickDecision(playedCards, startingPlayerIndex);
        std::cout << "Vítěz je #" << fst << " s vítěznou kartou " << snd.toString() << std::endl;
        trickWinner = fst;
        trickWinnerSet = true;
        waitingForTrickEnd = true;

        // Kontrola prohry v BETL/DURCH
        if (gameLogic.getMode() == Mode::BETL) {
            if (trickWinner == licitator->getNumber()) {
                bool loss = false;
                result = gameResult(&loss);
                state = State::END;
                return true;
            }
        } else {
            if (trickWinner != licitator->getNumber()) {
                bool loss = false;
                result = gameResult(&loss);
                state = State::END;
                return true;
            }
        }

        for (auto wonCard : playedCards) {
            players[trickWinner]->addWonCard(wonCard.second);
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
        result = gameResult(&win);
        state = State::END;
    }

    nextPlayer();
    return true;
}
// GAME_RESULT - vyhodnocení výsledku hry
std::pair<int, int> Game::gameResult(bool* result) {
    if (result != nullptr) {
        if (*result) {
            return {1, 0};
        }
        return {0, 1};
    }

    int playersPoints = 0;
    int licitatorPoints = licitator->calculateHand(gameLogic.getMode());

    for (Player* player : players) {
        if (player != licitator) {
            playersPoints += player->calculateHand(gameLogic.getMode());
        }
    }

    if (licitator->getNumber() == trickWinner) {
        licitatorPoints += 10;
    } else {
        playersPoints += 10;
    }

    return {licitatorPoints, playersPoints};
}

bool Game::gameHandler(Card &card, std::string &label) {
    State oldState = state;
    bool result = true;
    switch (static_cast<int>(state)) {
        case 1: gameState1(card); break;
        case 2: gameState2(card); break;
        case 3: gameState3(label); break;
        case 4: gameState4(label); break;
        case 5: gameState5(label); break;
        case 6: result = gameState6(card); break;
        case 7: result = gameState7(card); break;
        default: std::cout << "ERROR: Unknown state" << std::endl; break;
    }
    if (oldState != state) {
        stateChanged = 1;
    }

    return result;
}