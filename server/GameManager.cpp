#include "GameManager.hpp"
#include "MessageHandler.hpp"
#include "NetworkManager.hpp"
#include "Protocol.hpp"
#include <iostream>
#include <random>

GameManager::GameManager(int requiredPlayers, NetworkManager* networkManager, ClientManager* clientManager)
    : networkManager(networkManager), clientManager(clientManager), requiredPlayers(requiredPlayers) {

    std::cout << "🔧 GameManager vytvořen (požadováno " << requiredPlayers << " hráčů)" << std::endl;
}

GameManager::~GameManager() {
    game = nullptr;
}

void GameManager::initPlayers() {
    std::lock_guard<std::mutex> lock(gameMutex);
    for (auto client : clientManager->getClients()) {
        game->initPlayer(client->playerNumber, client->nickname);
    }
}

void GameManager::startGame() {
    std::cout << "\n" << std::string(50, '=') << std::endl;
    std::cout << "🎮 SPOUŠTÍM HERNÍ LOGIKU 🎮" << std::endl;
    std::cout << std::string(50, '=') << std::endl;

    game = std::make_unique<Game>(requiredPlayers);

    // ===== KROK 0: Inicializovat hráče =====
    initPlayers();

    // ===== KROK 1: Posílám 1. GAME_START =====
    std::cout << "\n📢 Hra se načítá..." << std::endl;

    for (int playerNum = 0; playerNum < requiredPlayers; playerNum++) {
        clientManager->sendToPlayer(playerNum, Protocol::MessageType::GAME_START, {});
        std::cout << "✓ Hráč dostal záznam o začátku hry " << playerNum << std::endl;
    }

    // ===== KROK 2: Čekání 5 sekund =====
    std::cout << "\n⏳ Čekám 5 sekund před rozdáním karet..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(5));
    std::cout << "✓ Čekání dokončeno" << std::endl;

    // ===== KROK 3 Inicializace hry a rozdání karet =====
    {
        std::cout << "\n🃏 Rozdávám karty hráčům..." << std::endl;
        std::lock_guard<std::mutex> lock(gameMutex);
        //std::mt19937 generator(static_cast<unsigned int>(std::time(0)));
        //std::uniform_int_distribution<int> distribution(0, requiredPlayers - 1);
        game->defineLicitator(0);//distribution(generator));
        game->dealCards();
        std::cout << "✓ Karty rozdány" << std::endl;
    }

    // ===== KROK 4: Odeslat GAME_START s daty =====
    std::cout << "\n📢 Posílám GAME_START všem hráčům..." << std::endl;

    for (int playerNum = 0; playerNum < requiredPlayers; playerNum++) {
        std::vector<std::string> gameData = serializeGameStart(playerNum);
        clientManager->sendToPlayer(playerNum, Protocol::MessageType::GAME_START, gameData);
        std::cout << "✓ GAME_START odesláno hráči " << playerNum << std::endl;
    }

    std::cout << "\n" << std::string(50, '=') << std::endl;
    std::cout << "✅ Hra úspěšně spuštěna!" << std::endl;
    std::cout << std::string(50, '=') << std::endl;

    // ===== KROK 5: Odeslat GAME_STATE =====
    std::cout << "\n📢 Posílám GAME_STATE všem hráčům..." << std::endl;

    for (int playerNum = 0; playerNum < requiredPlayers; playerNum++) {
        sendGameStateToPlayer(playerNum);
        std::cout << "✓ GAME_START odesláno hráči " << playerNum << std::endl;
    }

    {
        std::lock_guard<std::mutex> lock(gameMutex);
        game->stateChanged = 0;
    }
}

// ============================================================
// SERIALIZACE
// ============================================================
std::vector<std::string> GameManager::serializeGameState() {
    // state|stateChanged|gameStarted|mode|trumph|isPlayedCards|cards|change_trick
    std::vector<std::string> gameState;

    gameState.emplace_back(std::to_string(static_cast<int>(game->getState())));
    gameState.emplace_back(std::to_string(game->stateChanged));
    gameState.emplace_back(std::to_string(game->gameStarted));

    if (game->gameStarted) {
        gameState.emplace_back(std::to_string(static_cast<int>(game->getGameLogic().getMode())));
        if (game->getGameLogic().getMode() == Mode::HRA) {
            gameState.emplace_back(suitToString(game->getGameLogic().getTrumph().value()));
        } else {
            gameState.emplace_back("");
        }

        if (!game->getPlayedCards().empty()) {
            gameState.emplace_back("1"); // isPlayedCards
            std::string cardsArray;
            for (auto map : game->getPlayedCards()) {
                std::cout << "PlayedCard  - " << map.second.toString() << std::endl;
                cardsArray += map.second.toString();
                cardsArray += ":";
            }
            gameState.emplace_back(cardsArray);
            int changeTrick = game->isWaitingForTrickEnd() ? 1 : 0;
            gameState.emplace_back(std::to_string(changeTrick));
        } else {
            gameState.emplace_back("0" ); // isPlayedCards
        }
    }

    return gameState;
}

std::vector<std::string> GameManager::serializeGameStart(int playerNumber) {
    // <PLAYER>|<players>|<licitator>|<activePlayer>
    std::vector<std::string> gameData;
    gameData.push_back(serializePlayer(playerNumber));
    std::string playersArray;
    for (auto player : game->getPlayers()) {
        if (playerNumber != player->getNumber()) {
            playersArray += std::to_string(player->getNumber()) + "-" + player->getNick();
            playersArray += ":";
        }
    }
    gameData.push_back(playersArray);
    gameData.push_back(std::to_string(game->getLicitator()->getNumber()));
    gameData.push_back(std::to_string(game->getActivePlayer()->getNumber()));

    return gameData;
}

std::string GameManager::serializePlayer(int playerNumber) {
    // <number>-<nickname>|<cards>
    Player* player = game->getPlayer(playerNumber);
    if (!player) return "NONE";

    std::string msg;
    msg += std::to_string(player->getNumber());
    msg += "-";
    msg += player->getNick();
    msg += "|";

    for (auto& card : player->getHand().getCards()) {
        msg += card.toString();
        msg += ":";
    }

    return msg;
}

std::vector<std::string> GameManager::serializeInvalid(int playerNumber) {
    std::vector<std::string> msg;
    Player* player = game->getPlayer(playerNumber);
    msg.emplace_back(player->getInvalidMove());
    return msg;
}

// ============================================================
// HERNÍ LOGIKA - SendGameState, NotifyActivePlayer
// ============================================================

void GameManager::sendInvalidPlayer(int playerNumber) {
    std::cout << "📤 Posílám neplatný tah hráči #" << playerNumber << std::endl;

    std::lock_guard<std::mutex> lock(gameMutex);

    std::vector<std::string> msg = serializeInvalid(playerNumber);
    clientManager->sendToPlayer(playerNumber, Protocol::MessageType::INVALID, msg);

    std::cout << "✅ Chybný tah odeslán hráči #" << playerNumber << std::endl;
}

void GameManager::sendGameStateToPlayer(int playerNumber) {
    std::cout << "📤 Posílám stav hry hráči #" << playerNumber << std::endl;

    std::lock_guard<std::mutex> lock(gameMutex);

    std::vector<std::string> gameState = serializeGameState();
    clientManager->sendToPlayer(playerNumber, Protocol::MessageType::STATE, gameState);

    std::cout << "✅ Stav hry odeslán hráči #" << playerNumber << std::endl;
}

void GameManager::notifyActivePlayer() {
    std::lock_guard<std::mutex> lock(gameMutex);

    if (!game) {
        std::cerr << "⚠ Hra není inicializována" << std::endl;
        return;
    }

    int activePlayer = game->getActivePlayer()->getNumber();

    std::cout << "🔔 Notifikuji hráče #" << activePlayer << " že je na tahu" << std::endl;

    std::vector<std::string> turnData;
    turnData.emplace_back("Je váš tah");
    turnData.emplace_back(std::to_string(activePlayer));

    clientManager->sendToPlayer(activePlayer, Protocol::MessageType::YOUR_TURN, turnData);

    std::cout << "✅ YOUR_TURN odesláno hráči #" << activePlayer << std::endl;
}

// ============================================================
// ZPRACOVÁNÍ POŽADAVKŮ OD KLIENTA
// ============================================================

void GameManager::handleTrick(ClientInfo* client) {
    std::unique_lock<std::mutex> lock(trickMutex);
    trickResponses++;

    std::cout << "✓ TRICK od hráče #" << client->playerNumber
              << " (" << trickResponses << "/" << requiredPlayers << ")" << std::endl;

    if (trickResponses == requiredPlayers) {
        std::cout << "🎯 Všichni hráči potvrdili štych" << std::endl;

        {
            std::lock_guard<std::mutex> gameLock(gameMutex);
            game->resetTrick(game->getTrickWinner());
        }

        trickResponses = 0;
        notifyActivePlayer();
    }
}

void GameManager::handleBidding(std::string& label) {
    {
        std::cout << "Mění se stav hry..." << std::endl;
        std::lock_guard<std::mutex> lock(gameMutex);
        Card* card = nullptr;
        game->gameHandler(*card, label);
        std::cout << "Změna dokončena." << std::endl;
    }

    std::vector<Player*> players;
    {
        std::lock_guard<std::mutex> lock(gameMutex);
        players = game->getPlayers();
    }

    for (auto player : players) {
        sendGameStateToPlayer(player->getNumber());
    }

    if (label == "BETL" || label == "DURCH") {
        for (auto player : players) {
            std::vector<std::string> data = serializeGameStart(player->getNumber());
            clientManager->sendToPlayer(player->getNumber(), Protocol::MessageType::GAME_START, data);
        }
    } else {
        std::lock_guard<std::mutex> lock(gameMutex);
        game->stateChanged = 0;
    }

    if (game->getState() == State::LICITACE_TALON) {
        std::string clientData = serializePlayer(game->getActivePlayer()->getNumber());
        clientManager->sendToPlayer(game->getActivePlayer()->getNumber(), Protocol::MessageType::CLIENT_DATA, {clientData});
    }
}

void GameManager::handleCard(Card card) {
    std::string null;
    bool result;
    int actualActivePlayerNumber;
    {
        std::lock_guard<std::mutex> lock(gameMutex);
        actualActivePlayerNumber = game->getActivePlayer()->getNumber();
    }

    {
        std::lock_guard<std::mutex> lock(gameMutex);
        result = game->gameHandler(card, null);
    }

    if (result) {
        std::vector<Player*> players;
        {
            std::lock_guard<std::mutex> lock(gameMutex);
            players = game->getPlayers();
        }

        for (auto player : players) {
            sendGameStateToPlayer(player->getNumber());
        }

        {
            std::lock_guard<std::mutex> lock(gameMutex);
            game->stateChanged = 0;
        }
        std::string clientData = serializePlayer(actualActivePlayerNumber);
        clientManager->sendToPlayer(actualActivePlayerNumber, Protocol::MessageType::CLIENT_DATA, {clientData});

        if (game->getState() == State::END) {
            clientManager->nullreadyCount();
            std::pair<int, int> res = game->getResult();
            std::string gameResult = std::to_string(res.first) + ":" + std::to_string(res.second);
            clientManager->broadcastMessage(Protocol::MessageType::RESULT, {gameResult});
        }

        if (!game->isWaitingForTrickEnd()) {
            notifyActivePlayer();
        }
    } else {
        int activePlayerNumber;
        {
            std::lock_guard<std::mutex> lock(gameMutex);
            activePlayerNumber = game->getActivePlayer()->getNumber();
        }
        sendInvalidPlayer(activePlayerNumber);
        notifyActivePlayer();
    }
}