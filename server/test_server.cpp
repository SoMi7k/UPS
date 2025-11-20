#include "test_server.hpp"
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <chrono>
#include <string>

#define QUEUE_LENGTH 10

// ============================================================
// KONSTRUKTOR A DESTRUKTOR
// ============================================================

GameServer::GameServer(int port)
    : serverSocket(-1),
      port(port),
      running(false),
      game(nullptr),
      connectedPlayers(0) {

    std::cout << "🔧 GameServer vytvořen na portu " << port << std::endl;
}

GameServer::~GameServer() {
    std::cout << "🗑️ GameServer destruktor - provádím cleanup..." << std::endl;

    if (running) {
        stop();
    }

    cleanup();
}

// ============================================================
// 1. INITIALIZE SOCKET - Inicializace serveru
// ============================================================

bool GameServer::initializeSocket() {
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        std::cerr << "Chyba při vytváření socketu" << std::endl;
        return false;
    }
    std::cout << "Socket vytvořen (fd: " << serverSocket << ")" << std::endl;

    int opt = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "Chyba při nastavení SO_REUSEADDR" << std::endl;
        close(serverSocket);
        return false;
    }
    std::cout << "SO_REUSEADDR nastaven" << std::endl;

    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    std::cout << "Adresa serveru: ??? " << port << std::endl;

    if (bind(serverSocket,
             reinterpret_cast<sockaddr*>(&serverAddress),
             sizeof(serverAddress)) < 0) {
        std::cerr << "Chyba při bindování socketu (port možná už používán)" << std::endl;
        close(serverSocket);
        return false;
    }
    std::cout << "Socket nabindován na port " << port << std::endl;

    if (listen(serverSocket, QUEUE_LENGTH) < 0) {
        std::cerr << "Chyba při naslouchání" << std::endl;
        close(serverSocket);
        return false;
    }
    std::cout << "Server naslouchá na portu " << port << std::endl;

    return true;
}

// ============================================================
// 2. ACCEPT CLIENTS - Přijímání nových klientů
// ============================================================

void GameServer::acceptClients() {
    std::cout << "\n=== Čekám na připojení klientů ===" << std::endl;

    while (running) {
        sockaddr_in clientAddress{};
        socklen_t clientLen = sizeof(clientAddress);

        int clientSocket = accept(serverSocket,
                                  reinterpret_cast<sockaddr*>(&clientAddress),
                                  &clientLen);

        if (clientSocket < 0) {
            if (running) {
                std::cerr << "Chyba při přijímání klienta" << std::endl;
            }
            continue;
        }

        char clientIP[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientAddress.sin_addr, clientIP, INET_ADDRSTRLEN);

        std::cout << "\n✓ Nový klient se připojil!" << std::endl;
        std::cout << "  - Socket: " << clientSocket << std::endl;
        std::cout << "  - IP: " << clientIP << std::endl;
        std::cout << "  - Port: " << ntohs(clientAddress.sin_port) << std::endl;

        // Kontrola volného místa (ignorujeme dočasně odpojené)
        int activeConnections = 0;
        {
            std::lock_guard<std::mutex> lock(clientsMutex);
            for (auto* c : clients) {
                if (c && !c->isDisconnected) {
                    activeConnections++;
                }
            }
        }

        if (activeConnections >= requiredPlayers) {
            std::cout << "⚠ Hra je plná, odmítám klienta" << std::endl;
            sendMessage(clientSocket, messageType::ERROR, "Game is full");
            close(clientSocket);
            continue;
        }

        // Vytvoření nového klienta
        auto* client = new ClientInfo{
            clientSocket,
            connectedPlayers, // Může být přepsáno při reconnectu
            std::string(clientIP),
            true,
            std::thread(),
            generateSessionId(),
            std::chrono::steady_clock::now(),
            false,
            ""
        };

        // Přidání do seznamu
        {
            std::lock_guard<std::mutex> lock(clientsMutex);
            clients.push_back(client);
            connectedPlayers++;
        }

        // Spuštění vlákna
        client->clientThread = std::thread(&GameServer::handleClient, this, client);
        client->clientThread.detach();

        std::cout << "✓ Vlákno pro hráče #" << client->playerNumber << " spuštěno" << std::endl;

        // Pokud máme všechny hráče, spustíme hru
        if (connectedPlayers == requiredPlayers) {
            std::cout << "\n🎮 Všichni hráči připojeni - spouštím hru!" << std::endl;
            startGame();
        }

    }
}

// ============================================================
// 3. HANDLE CLIENT - Obsluha jednoho klienta
// ============================================================

void GameServer::handleClient(ClientInfo* client) {
    std::cout << "\n>>> Vlákno pro hráče #" << client->playerNumber << " zahájeno <<<" << std::endl;

    // ===== KROK 1: Poslání WELCOME zprávy =====
    nlohmann::json welcomeData = {};
    welcomeData["playerNumber"] = client->playerNumber;
    welcomeData["sessionId"] = client->sessionId;
    welcomeData["lobby"] = 1;
    welcomeData["requiredPlayers"] = requiredPlayers;
    sendMessage(client->socket, messageType::WELCOME, welcomeData.dump());

    // ===== KROK 2: Čekání na NICKNAME nebo RECONNECT =====
    std::string initialMessage = receiveMessage(client->socket);
    if (initialMessage.empty()) {
        std::cerr << "⚠ Hráč #" << client->playerNumber << " se odpojil před odesláním zprávy" << std::endl;
        handleClientDisconnection(client);
        return;
    }

    nlohmann::json msgJson = deserialize(initialMessage);
    if (msgJson.empty()) {
        handleClientDisconnection(client);
        return;
    }
    std::string msgType = msgJson["type"];

    // Kontrola zda jde o RECONNECT
    if (msgType == messageType::CONNECT && msgJson["data"].contains("sessionId")) {
        std::string sessionId = msgJson["data"]["sessionId"];
        std::cout << "🔄 Pokus o reconnect se session ID: " << sessionId << std::endl;

        ClientInfo* oldClient = findDisconnectedClient(sessionId);
        if (oldClient && reconnectClient(oldClient, client->socket)) {
            std::cout << "✅ Hráč #" << oldClient->playerNumber << " úspěšně reconnectnut" << std::endl;

            // Pošleme aktuální stav hry
            sendGameStateToPlayer(oldClient->playerNumber);
            nlohmann::json clientData;
            clientData["client"] = serializePlayer(oldClient->playerNumber);
            sendToPlayer(oldClient->playerNumber, messageType::CLIENT_DATA, clientData.dump());

            // Pokračujeme se starým clientem
            client = oldClient;
        } else {
            std::cerr << "❌ Reconnect selhal" << std::endl;
            nlohmann::json errorData;
            errorData["message"] = "Reconnect failed - session expired or invalid";
            sendMessage(client->socket, messageType::ERROR, errorData.dump());
            disconnectClient(client);
            return;
        }
    } else {
        // Běžný nový hráč
        std::string nickname = msgJson["data"]["nickname"];
        client->nickname = nickname;

        {
            std::lock_guard<std::mutex> lock(gameMutex);
            game->initPlayer(client->playerNumber, nickname);
        }

        std::cout << "  -> Nickname přijat od hráče #" << client->playerNumber << std::endl;

        nlohmann::json waitData;
        waitData["current"] = connectedPlayers;

        if (connectedPlayers < requiredPlayers) {
            sendMessage(client->socket, messageType::WAIT_LOBBY, waitData.dump());
            std::cout << "  -> WAIT_LOBBY odesláno hráči #" << client->playerNumber << std::endl;
        }
    }

    // ===== KROK 3: Hlavní smyčka =====
    std::cout << "  -> Vstupuji do příjmací smyčky pro hráče #" << client->playerNumber << std::endl;

    while (running && client->connected) {
        std::string message = receiveMessage(client->socket);
        if (message.empty()) {
            disconnectClient(client);
            return;
        }

        if (message.empty()) {
            std::cout << "\n⚠ Hráč #" << client->playerNumber << " ztratil spojení" << std::endl;
            handleClientDisconnection(client); // ZMĚNA
            break;
        }

        // Aktualizace last seen
        client->lastSeen = std::chrono::steady_clock::now();

        // Odstranění koncových znaků
        while (!message.empty() && (message.back() == '\n' || message.back() == '\r')) {
            message.pop_back();
        }

        std::cout << "\n📨 Od hráče #" << client->playerNumber << ": \"" << message << "\"" << std::endl;

        try {
            processClientMessage(client, message);
        } catch (const std::exception& e) {
            std::cerr << "❌ Výjimka při zpracování zprávy: " << e.what() << std::endl;
            nlohmann::json errorData;
            errorData["message"] = "Interní chyba serveru";
            sendMessage(client->socket, messageType::ERROR, errorData.dump());
        }
    }

    std::cout << "\n<<< Vlákno pro hráče #" << client->playerNumber << " končí >>>" << std::endl;
}

// ============================================================
// 4. START GAME - Spuštění hry
// ============================================================

void GameServer::startGame() {
    std::cout << "\n" << std::string(50, '=') << std::endl;
    std::cout << "🎮 SPOUŠTÍM HERNÍ LOGIKU 🎮" << std::endl;
    std::cout << std::string(50, '=') << std::endl;

    // ===== KROK 1: Posílám 1. GAME_START =====
    std::cout << "\n📢 Hra se načítá..." << std::endl;

    for (int playerNum = 0; playerNum < requiredPlayers; playerNum++) {
        sendToPlayer(playerNum, messageType::GAME_START, {});
        std::cout << "✓ Hráč dostal záznam o začátku hry " << playerNum << std::endl;
    }

    // ===== KROK 2: Čekání 5 sekund =====
    std::cout << "\n⏳ Čekám 5 sekund před rozdáním karet..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(5));
    std::cout << "✓ Čekání dokončeno" << std::endl;

    // ===== KROK 3 Inicializace hry a rozdání karet =====
    std::cout << "\n🃏 Rozdávám karty hráčům..." << std::endl;
    {
        std::lock_guard<std::mutex> lock(gameMutex);
        game->defineLicitator(0);
        game->dealCards();
    }
    std::cout << "✓ Karty rozdány" << std::endl;

    // ===== KROK 4: Odeslat GAME_START s daty =====
    std::cout << "\n📢 Posílám GAME_START všem hráčům..." << std::endl;

    for (int playerNum = 0; playerNum < requiredPlayers; playerNum++) {
        nlohmann::json gameData = serializeGameStart(playerNum);
        sendToPlayer(playerNum, messageType::GAME_START, gameData.dump());
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
    game->stateChanged = 0;
}

// ============================================================
// 5. KOMUNIKAČNÍ FUNKCE
// ============================================================

bool GameServer::sendMessage(int socket, const std::string& msgType, const std::string& message) {
    try {
        nlohmann::json msg;
        msg["type"] = msgType;

        // Pokud message už je JSON string, parsuj ho
        // Pokud ne, ulož jako string
        try {
            msg["data"] = nlohmann::json::parse(message);
        } catch (...) {
            msg["data"] = message;
        }

        msg["timestamp"] = std::time(nullptr);

        std::string serialized = msg.dump() + "\n";
        ssize_t bytesSent = send(socket, serialized.c_str(), serialized.size(), 0);

        return bytesSent == (ssize_t)serialized.size();
    }
    catch (const std::exception& e) {
        std::cerr << "❌ Chyba odeslání: " << e.what() << std::endl;
        return false;
    }
}

void GameServer::broadcastMessage(const std::string& msgType, const std::string& message) {
    std::lock_guard<std::mutex> lock(clientsMutex);

    std::cout << "📢 Broadcast: " << msgType << std::endl;

    for (auto* client : clients) {
        if (client && client->connected) {
            sendMessage(client->socket, msgType, message);
        }
    }
}

void GameServer::sendToPlayer(int playerNumber, const std::string& msgType, const std::string& message) {
    std::lock_guard<std::mutex> lock(clientsMutex);

    for (auto* client : clients) {
        if (client && client->playerNumber == playerNumber && client->connected) {
            sendMessage(client->socket, msgType, message);
            return;
        }
    }

    std::cerr << "⚠ Hráč #" << playerNumber << " nebyl nalezen" << std::endl;
}

std::string GameServer::receiveMessage(int socket) {
    std::string buffer;
    char chunk[256];
    ssize_t bytesReceived;

    while (true) {
        bytesReceived = recv(socket, chunk, sizeof(chunk), 0);

        if (bytesReceived <= 0) {
            return "";  // Klient se odpojil nebo chyba
        }

        buffer.append(chunk, bytesReceived);

        // Hledáme konec zprávy (newline)
        size_t pos = buffer.find('\n');
        if (pos != std::string::npos) {
            std::string message = buffer.substr(0, pos);
            return message;
        }
    }
}

nlohmann::json GameServer::deserialize(const std::string& msg) {
    if (msg.empty()) {
        return nlohmann::json{};
    }

    try {
        nlohmann::json parsed = nlohmann::json::parse(msg);
        std::cout << "📥 Typ zprávy: " << parsed["type"] << std::endl;

        if (parsed.contains("data")) {
            std::cout << "📥 Data: " << parsed["data"] << std::endl;
        }
        return parsed;

    } catch (const std::exception& e) {
        std::cerr << "❌ Chyba při parsování JSON: " << e.what() << std::endl;
        return nlohmann::json{};
    }
}

// ============================================================
// 6. POMOCNÉ FUNKCE - DEFINICE (doplníte logiku)
// ============================================================
nlohmann::json GameServer::serializeGameState() {
    nlohmann::json state;
    state["state"] = game->getState();
    state["change_state"] = game->stateChanged;
    state["gameStarted"] = game->gameStarted;

    if (game->gameStarted) {
        state["mode"] = modeToString(game->getGameLogic().getMode());
        state["trump"] = suitToString(game->getGameLogic().getTrumph());
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

nlohmann::json GameServer::serializeGameStart(int playerNumber) {
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

nlohmann::json GameServer::serializePlayer(int playerNumber) {
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

nlohmann::json GameServer::serializeInvalid(int playerNumber) {
    nlohmann::json msg;
    Player* player = game->getPlayer(playerNumber);
    msg["data"] = player->getInvalidMove();

    return msg;
}

// ============================================================
// HLAVNÍ METODY - START, STOP, IS RUNNING
// ============================================================

void GameServer::start() {
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "🚀 SPOUŠTÍM SERVER" << std::endl;
    std::cout << std::string(60, '=') << std::endl;

    // Inicializace socketu
    if (!initializeSocket()) {
        std::cerr << "❌ Nepodařilo se inicializovat socket" << std::endl;
        return;
    }

    running = true;
    game = std::make_unique<Game>(requiredPlayers);

    // Spuštění accept threadu
    std::cout << "\n🔄 Spouštím accept thread..." << std::endl;
    acceptThread = std::thread(&GameServer::acceptClients, this);

    // Spuštění timeout checkeru
    std::thread timeoutThread(&GameServer::checkDisconnectedClients, this);
    timeoutThread.detach();

    std::cout << "\n✅ Server úspěšně spuštěn!" << std::endl;
    std::cout << "📡 Naslouchám na portu " << port << std::endl;
    std::cout << "⏳ Čekám na " << requiredPlayers << " hráče..." << std::endl;
    std::cout << std::string(60, '=') << std::endl;

    // Čekáme na dokončení accept threadu (blocking)
    if (acceptThread.joinable()) {
        acceptThread.join();
    }

    std::cout << "\n🛑 Server ukončen" << std::endl;
}

void GameServer::stop() {
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "🛑 ZASTAVUJI SERVER" << std::endl;
    std::cout << std::string(60, '=') << std::endl;

    running = false;

    // Zavření hlavního socketu (ukončí accept loop)
    if (serverSocket >= 0) {
        std::cout << "🔌 Zavírám hlavní socket..." << std::endl;
        shutdown(serverSocket, SHUT_RDWR);
        close(serverSocket);
        serverSocket = -1;
    }

    // Odpojení všech klientů
    {
        std::lock_guard<std::mutex> lock(clientsMutex);
        std::cout << "🔌 Odpojuji " << clients.size() << " klientů..." << std::endl;

        for (auto* client : clients) {
            if (client && client->connected) {
                // Pošleme DISCONNECT zprávu
                sendMessage(client->socket, messageType::DISCONNECT, "Server se vypíná");

                // Zavřeme socket
                shutdown(client->socket, SHUT_RDWR);
                close(client->socket);
                client->connected = false;
            }
        }
    }

    // Počkáme na dokončení accept threadu
    if (acceptThread.joinable()) {
        std::cout << "⏳ Čekám na dokončení accept threadu..." << std::endl;
        acceptThread.join();
    }

    std::cout << "✅ Server zastaven" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
}

bool GameServer::isRunning() const {
    return running;
}

// ============================================================
// GETTERY
// ============================================================

int GameServer::getConnectedPlayers() const {
    return connectedPlayers;
}

int GameServer::getRequiredPlayers() const {
    return requiredPlayers;
}

// ============================================================
// CLEANUP - Úklid zdrojů
// ============================================================

void GameServer::cleanup() {
    std::cout << "🧹 Provádím cleanup..." << std::endl;

    // Vyčištění seznamu klientů
    {
        std::lock_guard<std::mutex> lock(clientsMutex);

        for (auto* client : clients) {
            if (client) {
                // Pokud je vlákno ještě aktivní, odpojíme ho
                if (client->clientThread.joinable()) {
                    client->clientThread.detach();
                }

                // Zavřeme socket
                if (client->socket >= 0) {
                    close(client->socket);
                }

                // Uvolníme paměť
                delete client;
            }
        }

        clients.clear();
        connectedPlayers = 0;
    }

    // Vymazání herní instance
    {
        std::lock_guard<std::mutex> lock(gameMutex);
        game.reset();
    }

    std::cout << "✅ Cleanup dokončen" << std::endl;
}

// ============================================================
// DISCONNECT CLIENT - Odpojení jednoho klienta
// ============================================================

void GameServer::disconnectClient(ClientInfo* client) {
    if (!client) {
        return;
    }

    std::cout << "\n" << std::string(50, '-') << std::endl;
    std::cout << "🔌 Odpojuji hráče #" << client->playerNumber << std::endl;
    std::cout << "  - IP: " << client->address << std::endl;
    std::cout << "  - Socket: " << client->socket << std::endl;

    // Označíme jako odpojeného
    client->connected = false;

    // Zavřeme socket
    if (client->socket >= 0) {
        shutdown(client->socket, SHUT_RDWR);
        close(client->socket);
        client->socket = -1;
    }

    // Odstraníme ze seznamu klientů
    {
        std::lock_guard<std::mutex> lock(clientsMutex);

        auto it = std::find(clients.begin(), clients.end(), client);
        if (it != clients.end()) {
            clients.erase(it);
            connectedPlayers--;

            std::cout << "  - Odstraněn ze seznamu" << std::endl;
            std::cout << "  - Zbývá " << connectedPlayers << "/" << requiredPlayers << " hráčů" << std::endl;
        }
    }

    // Notifikujeme ostatní hráče
    {
        nlohmann::json statusData;
        statusData["message"] = "Hráč se odpojil";
        statusData["playerNumber"] = client->playerNumber;
        statusData["connectedPlayers"] = connectedPlayers;

        broadcastMessage(messageType::STATUS, statusData.dump());
    }

    std::cout << "✅ Hráč #" << client->playerNumber << " odpojen" << std::endl;
    std::cout << std::string(50, '-') << std::endl;

    // TODO: Pokud chcete, můžete zde přidat logiku pro:
    {
        nlohmann::json waitData;
        waitData["current"] = connectedPlayers;
        broadcastMessage(messageType::WAIT_LOBBY, waitData.dump());
        std::cout << "  -> WAIT_LOBBY odesláno hráči #" << client->playerNumber << std::endl;
    }

    game->resetGame();

    // - Ukončení hry pokud se odpojí hráč uprostřed
    // - Reset serveru
    // - atd.
}

// ============================================================
// PROCESS CLIENT MESSAGE - Zpracování zpráv od klienta
// ============================================================

void GameServer::processClientMessage(ClientInfo* client, const std::string& message) {
    nlohmann::json msg = deserialize(message);
    if (msg.empty()) {
        disconnectClient(client);
        return;
    }

    if (msg.empty()) {
        std::cerr << "⚠ Nepodařilo se parsovat zprávu" << std::endl;
        nlohmann::json errorData;
        errorData["message"] = "Neplatný formát zprávy";
        sendMessage(client->socket, messageType::ERROR, errorData.dump());
        return;
    }

    std::string msgType = msg["type"];
    nlohmann::json data = msg["data"];
    std::cout << "🔄 Zpracovávám zprávu typu: " << msgType
              << " od hráče #" << client->playerNumber << std::endl;


    // ===== HEARTBEAT =====
    if (msgType == messageType::HEARTBEAT) {
        std::cout << "💓 Heartbeat od hráče #" << client->playerNumber << std::endl;
        client->lastSeen = std::chrono::steady_clock::now();
    }

    // ===== READY =====
    else if (msgType == messageType::READY) {
        std::cout << "✅ Hráč #" << client->playerNumber << " je připraven" << std::endl;

        nlohmann::json okData;
        okData["message"] = "Připraven přijat";
        sendMessage(client->socket, messageType::OK, okData.dump());
    }

    // ===== TRICK =====
    else if (msgType == messageType::TRICK) {
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

    // ===== CARD =====
    else if (msgType == messageType::CARD) {
        Card card = cardMapping(data["card"]);
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
            sendToPlayer(actualActivePlayerNumber, messageType::CLIENT_DATA, clientData.dump());

            if (game->getState() == State::END) {
                nlohmann::json gameResult;
                gameResult["gameResult"] = game->getResult();
                sendToPlayer(client->playerNumber, messageType::RESULT, gameResult.dump());
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

    // ===== BIDDING =====
    else if (msgType == messageType::BIDDING) {
        std::string label = data["label"];

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

        {
            std::lock_guard<std::mutex> lock(gameMutex);
            game->stateChanged = 0;
        }

        if (game->getState() == State::LICITACE_TALON) {
            nlohmann::json clientData;
            clientData["client"] = serializePlayer(game->getActivePlayer()->getNumber());
            sendToPlayer(game->getActivePlayer()->getNumber(), messageType::CLIENT_DATA, clientData.dump());
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
        notifyActivePlayer();
    }

    // ===== RESET =====
    else if (msgType == messageType::RESET) {
        std::cout << "🔄 Hráč #" << client->playerNumber << " žádá o reset" << std::endl;
        std::string reset = data["reset"];

        if (reset == "ANO") {
            // TODO - Přesun clienta do lobby
            // TODO - Reset Hry
            disconnectClient(client);
        } else {
            disconnectClient(client);
        }
    }

    // ===== DISCONNECT =====
    else if (msgType == messageType::DISCONNECT) {
        std::cout << "👋 Hráč #" << client->playerNumber << " se odpojuje" << std::endl;
        disconnectClient(client);
    }

    // ===== CONNECT =====
    else if (msgType == messageType::CONNECT) {
        std::cout << "📨 Přijato CONNECT" << std::endl;
    }

    // ===== UNKNOWN =====
    else {
        std::cerr << "⚠ Neznámý typ zprávy: " << msgType << std::endl;

        nlohmann::json errorData;
        errorData["message"] = "Neznámý typ zprávy";
        errorData["receivedType"] = msgType;
        sendMessage(client->socket, messageType::ERROR, errorData.dump());
    }
}

// ============================================================
// HERNÍ LOGIKA - SendGameState, NotifyActivePlayer
// ============================================================

void GameServer::sendInvalidPlayer(int playerNumber) {
    std::cout << "📤 Posílám neplatný tah hráči #" << playerNumber << std::endl;

    std::lock_guard<std::mutex> lock(gameMutex);

    nlohmann::json msg = serializeInvalid(playerNumber);
    sendToPlayer(playerNumber, messageType::INVALID, msg.dump());

    std::cout << "✅ Chybný tah odeslán hráči #" << playerNumber << std::endl;
}

void GameServer::sendGameStateToPlayer(int playerNumber) {
    std::cout << "📤 Posílám stav hry hráči #" << playerNumber << std::endl;

    std::lock_guard<std::mutex> lock(gameMutex);

    nlohmann::json gameState = serializeGameState();
    sendToPlayer(playerNumber, messageType::STATE, gameState.dump());

    std::cout << "✅ Stav hry odeslán hráči #" << playerNumber << std::endl;
}

void GameServer::notifyActivePlayer() {
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

    sendToPlayer(activePlayer, messageType::YOUR_TURN, turnData.dump());

    std::cout << "✅ YOUR_TURN odesláno hráči #" << activePlayer << std::endl;
}

std::string GameServer::generateSessionId() {
    static int counter = 0;
    auto now = std::chrono::system_clock::now().time_since_epoch().count();
    return "SESSION_" + std::to_string(now) + "_" + std::to_string(counter++);
}

ClientInfo* GameServer::findDisconnectedClient(const std::string& sessionId) {
    std::lock_guard<std::mutex> lock(clientsMutex);

    for (auto* client : clients) {
        if (client && client->isDisconnected && client->sessionId == sessionId) {
            auto elapsed = std::chrono::steady_clock::now() - client->lastSeen;
            auto seconds = std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();

            if (seconds < RECONNECT_TIMEOUT_SECONDS) {
                return client;
            }
        }
    }

    return nullptr;
}

bool GameServer::reconnectClient(ClientInfo* oldClient, int newSocket) {
    if (!oldClient) return false;

    std::cout << "🔄 Reconnecting hráče #" << oldClient->playerNumber << std::endl;

    // Zavřeme starý socket (pokud existuje)
    if (oldClient->socket >= 0) {
        close(oldClient->socket);
    }

    // Nastavíme nový socket
    oldClient->socket = newSocket;
    oldClient->connected = true;
    oldClient->isDisconnected = false;
    oldClient->lastSeen = std::chrono::steady_clock::now();

    // Notifikujeme ostatní hráče
    nlohmann::json statusData;
    statusData["message"] = "Hráč se znovu připojil";
    statusData["playerNumber"] = oldClient->playerNumber;
    statusData["nickname"] = oldClient->nickname;
    broadcastMessage(messageType::STATUS, statusData.dump());

    return true;
}

void GameServer::handleClientDisconnection(ClientInfo* client) {
    if (!client) return;

    std::cout << "\n🔌 Hráč #" << client->playerNumber << " se odpojil - čekám na reconnect" << std::endl;

    // Označíme jako dočasně odpojeného (NE odstranění ze seznamu)
    client->connected = false;
    client->isDisconnected = true;
    client->lastSeen = std::chrono::steady_clock::now();

    // Zavřeme socket
    if (client->socket >= 0) {
        shutdown(client->socket, SHUT_RDWR);
        close(client->socket);
        client->socket = -1;
    }

    // Notifikujeme ostatní hráče
    nlohmann::json statusData;
    statusData["message"] = "Hráč ztratil spojení - čekáme na reconnect";
    statusData["playerNumber"] = client->playerNumber;
    statusData["reconnectTimeout"] = RECONNECT_TIMEOUT_SECONDS;
    broadcastMessage(messageType::STATUS, statusData.dump());

    std::cout << "⏳ Čekám " << RECONNECT_TIMEOUT_SECONDS << "s na reconnect hráče #"
              << client->playerNumber << std::endl;
}

void GameServer::checkDisconnectedClients() {
    while (running) {
        std::this_thread::sleep_for(std::chrono::seconds(10)); // Kontrola každých 10s

        std::lock_guard<std::mutex> lock(clientsMutex);

        auto now = std::chrono::steady_clock::now();
        std::vector<ClientInfo*> toRemove;

        for (auto* client : clients) {
            if (client && client->isDisconnected) {
                auto elapsed = now - client->lastSeen;
                auto seconds = std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();

                if (seconds >= RECONNECT_TIMEOUT_SECONDS) {
                    std::cout << "⏱️ Timeout pro hráče #" << client->playerNumber
                              << " - odstraňuji permanentně" << std::endl;
                    toRemove.push_back(client);
                }
            }
        }

        // Permanentní odstranění hráčů s timeoutem
        for (auto* client : toRemove) {
            disconnectClient(client); // Nyní permanentní odstranění
        }
    }
}