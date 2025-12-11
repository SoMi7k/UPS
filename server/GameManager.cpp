#include "GameManager.hpp"
#include "MessageHandler.hpp"
#include "NetworkManager.hpp"
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
        nlohmann::json none = {};
        clientManager->sendToPlayer(playerNum, messageType::GAME_START, none.dump());
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
        nlohmann::json gameData = serializeGameStart(playerNum);
        clientManager->sendToPlayer(playerNum, messageType::GAME_START, gameData.dump());
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
nlohmann::json GameManager::serializeGameState() {
    nlohmann::json state;
    state["state"] = game->getState();
    state["change_state"] = game->stateChanged;
    state["gameStarted"] = game->gameStarted;

    if (game->gameStarted) {
        state["mode"] = modeToString(game->getGameLogic().getMode());
        if (game->getGameLogic().getMode() == Mode::HRA) {
            state["trump"] = suitToString(game->getGameLogic().getTrumph().value());
        } else {
            state["trump"] = "";
        }
        state["isPlayedCards"] = 0;

        if (!game->getPlayedCards().empty()) {
            state["isPlayedCards"] = 1;
            nlohmann::json cardsArray = nlohmann::json::array();
            for (auto map : game->getPlayedCards()) {
                std::cout << "PlayedCard  - " << map.second.toString() << std::endl;
                std::string str_card = map.second.toString();
                cardsArray.push_back(str_card);
            }
            state["played_cards"] = cardsArray;
            state["change_trick"] = (game->isWaitingForTrickEnd()) ? 1 : 0;
            /*
            if (game->getPlayedCards().size() == requiredPlayers) {
                state["winner"] = game->getTrickWinner();
            } else {
                state["winner"] = game->getTrickWinner();
            }
            */
        }
    }

    return state;
}

nlohmann::json GameManager::serializeGameStart(int playerNumber) {
    nlohmann::json gameData;
    gameData["client"] = serializePlayer(playerNumber);
    // Ostatní hráči
    nlohmann::json playersArray = nlohmann::json::array();

    for (auto player : game->getPlayers()) {
        if (playerNumber != player->getNumber()) {
            std::string playerInfo = std::to_string(player->getNumber()) + "-" + player->getNick();
            playersArray.push_back(playerInfo);
        }
    }
    gameData["players"] = playersArray;
    gameData["licitator"] = game->getLicitator()->getNumber();
    gameData["active_player"] = game->getActivePlayer()->getNumber();

    return gameData;
}

nlohmann::json GameManager::serializePlayer(int playerNumber) {
    nlohmann::json client;

    Player* player = game->getPlayer(playerNumber);
    if (player == nullptr) {
        client["error"] = "Invalid player number";
        return client;
    }

    // Příklad serializace – uprav podle struktury Player
    client["number"] = player->getNumber();
    client["nickname"] = player->getNick();

    // Pokud má hráč ruku (karty apod.)
    nlohmann::json cards = nlohmann::json::array();
    for (const auto& card : player->getHand().getCards()) {
        //std::cout << "  - " << card.toString() << std::endl;
        cards.push_back(card.toString());
    }
    client["hand"] = cards;

    return client;
}

nlohmann::json GameManager::serializeInvalid(int playerNumber) {
    nlohmann::json msg;
    Player* player = game->getPlayer(playerNumber);
    msg["data"] = player->getInvalidMove();

    return msg;
}


// ============================================================
// HERNÍ LOGIKA - SendGameState, NotifyActivePlayer
// ============================================================

void GameManager::sendInvalidPlayer(int playerNumber) {
    std::cout << "📤 Posílám neplatný tah hráči #" << playerNumber << std::endl;

    std::lock_guard<std::mutex> lock(gameMutex);

    nlohmann::json msg = serializeInvalid(playerNumber);
    clientManager->sendToPlayer(playerNumber, messageType::INVALID, msg.dump());

    std::cout << "✅ Chybný tah odeslán hráči #" << playerNumber << std::endl;
}

void GameManager::sendGameStateToPlayer(int playerNumber) {
    std::cout << "📤 Posílám stav hry hráči #" << playerNumber << std::endl;

    std::lock_guard<std::mutex> lock(gameMutex);

    nlohmann::json gameState = serializeGameState();
    clientManager->sendToPlayer(playerNumber, messageType::STATE, gameState.dump());

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

    nlohmann::json turnData;
    turnData["message"] = "Je váš tah";
    turnData["playerNumber"] = activePlayer;

    clientManager->sendToPlayer(activePlayer, messageType::YOUR_TURN, turnData.dump());

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
            nlohmann::json data = serializeGameStart(player->getNumber());
            clientManager->sendToPlayer(player->getNumber(), messageType::GAME_START, data.dump());
        }
    } else {
        std::lock_guard<std::mutex> lock(gameMutex);
        game->stateChanged = 0;
    }

    if (game->getState() == State::LICITACE_TALON) {
        nlohmann::json clientData;
        clientData["client"] = serializePlayer(game->getActivePlayer()->getNumber());
        clientManager->sendToPlayer(game->getActivePlayer()->getNumber(), messageType::CLIENT_DATA, clientData.dump());
    }
}

void GameManager::handleCard(ClientInfo* client, Card card) {
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
        nlohmann::json clientData;
        clientData["client"] = serializePlayer(actualActivePlayerNumber);
        clientManager->sendToPlayer(actualActivePlayerNumber, messageType::CLIENT_DATA, clientData.dump());

        if (game->getState() == State::END) {
            nlohmann::json gameResult;
            gameResult["gameResult"] = game->getResult();
            clientManager->broadcastMessage(messageType::RESULT, gameResult.dump());
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