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
    serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");

    std::cout << "Adresa serveru: 127.0.0.1:" << port << std::endl;

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

        std::cout << "Čekám na klienta..." << std::endl;

        int clientSocket = accept(serverSocket,
                                  reinterpret_cast<sockaddr*>(&clientAddress),
                                  &clientLen);

        if (clientSocket < 0) {
            if (running) {
                std::cerr << "Chyba při přijímání klienta" << std::endl;
            } else {
                std::cout << "Server se vypíná..." << std::endl;
            }
            continue;
        }

        char clientIP[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientAddress.sin_addr, clientIP, INET_ADDRSTRLEN);

        std::cout << "\n✓ Nový klient se připojil!" << std::endl;
        std::cout << "  - Socket: " << clientSocket << std::endl;
        std::cout << "  - IP: " << clientIP << std::endl;
        std::cout << "  - Port: " << ntohs(clientAddress.sin_port) << std::endl;

        // Kontrola, zda máme volné místo
        {
            std::lock_guard<std::mutex> lock(clientsMutex);

            if (connectedPlayers >= requiredPlayers) {
                std::cout << "⚠ Hra je plná, odmítám klienta" << std::endl;
                sendMessage(clientSocket, messageType::ERROR, "Game is full");
                close(clientSocket);
                continue;
            }
        }

        // Vytvoření info struktury pro klienta
        auto* client = new ClientInfo{
            clientSocket,
            connectedPlayers,
            std::string(clientIP),
            true,
            std::thread()
        };

        // Přidání klienta do seznamu
        {
            std::lock_guard<std::mutex> lock(clientsMutex);
            clients.push_back(client);
            connectedPlayers++;
        }

        std::cout << "✓ Hráč #" << client->playerNumber << " přidán" << std::endl;
        std::cout << "  Připojeno: " << connectedPlayers << "/" << requiredPlayers << " hráčů" << std::endl;

        // Spuštění vlákna pro obsluhu klienta
        client->clientThread = std::thread(&GameServer::handleClient, this, client);
        client->clientThread.detach();

        std::cout << "✓ Vlákno pro hráče #" << client->playerNumber << " spuštěno" << std::endl;

        // Pokud máme všechny hráče, spustíme hru
        if (connectedPlayers == requiredPlayers) {
            std::cout << "\n🎮 Všichni hráči připojeni - spouštím hru!" << std::endl;
            startGame();
        }
    }

    std::cout << "Accept loop ukončen" << std::endl;
}

// ============================================================
// 3. HANDLE CLIENT - Obsluha jednoho klienta
// ============================================================

void GameServer::handleClient(ClientInfo* client) {
    std::cout << "\n>>> Vlákno pro hráče #" << client->playerNumber << " zahájeno <<<" << std::endl;

    // ===== KROK 1: Poslání WELCOME zprávy =====
    nlohmann::json welcomeData = {};
    welcomeData["data"]["playerNumber"] = client->playerNumber;
    sendMessage(client->socket, messageType::WELCOME, welcomeData.dump());
    std::cout << "  -> WELCOME odesláno hráči #" << client->playerNumber << std::endl;

    // ===== KROK 2: Čekání na NICKNAME od klienta =====
    std::string nicknameMessage = receiveMessage(client->socket);

    if (nicknameMessage.empty()) {
        std::cerr << "⚠ Hráč #" << client->playerNumber << " se odpojil před odesláním nicknamu" << std::endl;
        disconnectClient(client);
        return;
    }

    // TODO: Parsovat JSON a získat nickname
    // MÍSTO PRO VÁŠ KÓD:
    nlohmann::json nicknameJson = deserialize(nicknameMessage);
    std::string nickname = nicknameJson["data"]["nickname"];

    {
        std::lock_guard<std::mutex> lock(gameMutex);
        game->initPlayer(client->playerNumber, nickname);
    }

    std::cout << "  -> Nickname přijat od hráče #" << client->playerNumber << std::endl;

    // ===== KROK 3: Pokud ještě nemáme všechny hráče, pošleme WAIT_LOBBY =====
    if (connectedPlayers < requiredPlayers) {
        // TODO: Poslat WAIT_LOBBY zprávu
        // MÍSTO PRO VÁŠ KÓD:
        nlohmann::json waitData;
        waitData["current"] = connectedPlayers;
        waitData["required"] = requiredPlayers;
        sendMessage(client->socket, messageType::WAIT_LOBBY, waitData.dump());

        std::cout << "  -> WAIT_LOBBY odesláno hráči #" << client->playerNumber << std::endl;
    }

    // ===== KROK 4: Hlavní smyčka - příjem zpráv od klienta =====
    std::cout << "  -> Vstupuji do příjmací smyčky pro hráče #" << client->playerNumber << std::endl;

    while (running && client->connected) {
        std::string message = receiveMessage(client->socket);

        if (message.empty()) {
            std::cout << "\n⚠ Hráč #" << client->playerNumber << " se odpojil" << std::endl;
            break;
        }

        // Odstranění koncových znaků
        while (!message.empty() && (message.back() == '\n' || message.back() == '\r')) {
            message.pop_back();
        }

        std::cout << "\n📨 Od hráče #" << client->playerNumber << ": \"" << message << "\"" << std::endl;

        // Zpracování zprávy
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
    disconnectClient(client);
}

// ============================================================
// 4. START GAME - Spuštění hry
// ============================================================

void GameServer::startGame() {
    std::cout << "\n" << std::string(50, '=') << std::endl;
    std::cout << "🎮 SPOUŠTÍM HERNÍ LOGIKU 🎮" << std::endl;
    std::cout << std::string(50, '=') << std::endl;

    // ===== KROK 1: Inicializace hry a rozdání karet =====
    std::cout << "\n🃏 Rozdávám karty hráčům..." << std::endl;

    {
        std::lock_guard<std::mutex> lock(gameMutex);
        game->defineLicitator(std::rand() % requiredPlayers);
        game->dealCards();
    }

    std::cout << "✓ Karty rozdány" << std::endl;
    
    // ===== KROK 2: Odeslat GAME_START všem hráčům =====
    std::cout << "\n📢 Posílám GAME_START všem hráčům..." << std::endl;

    for (int playerNum = 0; playerNum < requiredPlayers; playerNum++) {
        nlohmann::json gameData = serializeGameStart(playerNum);
        sendToPlayer(playerNum, messageType::GAME_START, gameData.dump());
        std::cout << "✓ GAME_START odesláno" << std::endl;
    }

    // ===== KROK 3: Čekání 5 sekund =====
    std::cout << "\n⏳ Čekám 5 sekund před rozdáním karet..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(5));
    std::cout << "✓ Čekání dokončeno" << std::endl;

    std::cout << "\n" << std::string(50, '=') << std::endl;
    std::cout << "✅ Hra úspěšně spuštěna!" << std::endl;
    std::cout << std::string(50, '=') << std::endl;
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

void GameServer::processClientMessage(ClientInfo* client, const std::string& message) {
    // TODO: Implementovat zpracování různých typů zpráv
    // MÍSTO PRO VÁŠ KÓD:

    nlohmann::json msg = deserialize(message);

    if (msg.empty()) {
        std::cerr << "⚠ Nepodařilo se parsovat zprávu" << std::endl;
        return;
    }

    std::string msgType = msg["type"];

    std::cout << "🔄 Zpracovávám zprávu typu: " << msgType << std::endl;

    // Zde přidáte logiku podle typu zprávy
    // if (msgType == messageType::CARD) { ... }
    // if (msgType == messageType::BIDDING) { ... }
    // atd.
}

nlohmann::json GameServer::serializeGameState() {
    // TODO: Implementovat serializaci stavu hry
    nlohmann::json state;
    state["state"] = game->getState();
    state["change_trick"] = state["change_trick"] = (game->getPlayedCards().size() == 3) ? 1 : 0;
    state["change_state"] = "0";

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
    nlohmann::json hand;

    Player* player = game->getPlayer(playerNumber);
    if (player == nullptr) {
        hand["error"] = "Invalid player number";
        return hand;
    }

    // Příklad serializace – uprav podle struktury Player
    hand["number"] = player->getNumber();
    hand["nickname"] = player->getNick();

    // Pokud má hráč ruku (karty apod.)
    nlohmann::json cards = nlohmann::json::array();
    for (const auto& card : player->getHand().getCards()) {
        cards.push_back(card.toString());
    }
    hand["hand"] = cards;

    return hand;
}

nlohmann::json GameServer::serializeCard(const Card& card) {
    std::string msg = card.toString();
    return msg;
}

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
    // - Ukončení hry pokud se odpojí hráč uprostřed
    // - Reset serveru
    // - atd.
}

// ============================================================
// PROCESS CLIENT MESSAGE - Zpracování zpráv od klienta
// ============================================================
/*
void GameServer::processClientMessage(ClientInfo* client, const std::string& message) {
    nlohmann::json msg = deserialize(message);

    if (msg.empty()) {
        std::cerr << "⚠ Nepodařilo se parsovat zprávu" << std::endl;

        nlohmann::json errorData;
        errorData["message"] = "Neplatný formát zprávy";
        sendMessage(client->socket, messageType::ERROR, errorData.dump());
        return;
    }

    std::string msgType = msg["type"];
    std::cout << "🔄 Zpracovávám zprávu typu: " << msgType << " od hráče #" << client->playerNumber << std::endl;

    // TODO: Implementovat podle vašich potřeb
    // Zde je základní struktura:

    if (msgType == messageType::HEARTBEAT) {
        // Heartbeat - pouze potvrdíme příjem
        std::cout << "💓 Heartbeat od hráče #" << client->playerNumber << std::endl;
        // Neposíláme odpověď, klient jen kontroluje že je spojení aktivní
    }
    else if (msgType == messageType::READY) {
        // Hráč je připraven
        std::cout << "✅ Hráč #" << client->playerNumber << " je připraven" << std::endl;

        nlohmann::json okData;
        okData["message"] = "Připraven přijat";
        sendMessage(client->socket, messageType::OK, okData.dump());
    }
    else if (msgType == messageType::CARD) {
        // Hráč zahrál kartu
        std::cout << "🃏 Hráč #" << client->playerNumber << " zahrál kartu" << std::endl;

        // TODO: Zpracovat tah
        // std::string cardStr = msg["data"]["card"];
        // game->playCard(client->playerNumber, cardStr);
        // sendGameState();
        // notifyActivePlayer();
    }
    else if (msgType == messageType::BIDDING) {
        // Hráč licitoval
        std::cout << "💰 Hráč #" << client->playerNumber << " licituje" << std::endl;

        // TODO: Zpracovat licitaci
        // int bid = msg["data"]["bid"];
        // game->processBid(client->playerNumber, bid);
    }
    else if (msgType == messageType::RESET) {
        // Hráč chce reset
        std::cout << "🔄 Hráč #" << client->playerNumber << " žádá o reset" << std::endl;

        // TODO: Implementovat reset logiku
    }
    else if (msgType == messageType::DISCONNECT) {
        // Hráč se chce odpojit
        std::cout << "👋 Hráč #" << client->playerNumber << " se odpojuje" << std::endl;
        client->connected = false;
    }
    else {
        // Neznámý typ zprávy
        std::cerr << "⚠ Neznámý typ zprávy: " << msgType << std::endl;

        nlohmann::json errorData;
        errorData["message"] = "Neznámý typ zprávy";
        errorData["receivedType"] = msgType;
        sendMessage(client->socket, messageType::INVALID, errorData.dump());
    }
}
*/
// ============================================================
// HERNÍ LOGIKA - SendGameState, NotifyActivePlayer
// ============================================================

void GameServer::sendGameState() {
    std::cout << "📤 Posílám stav hry všem hráčům..." << std::endl;

    std::lock_guard<std::mutex> lock(gameMutex);

    if (!game) {
        std::cerr << "⚠ Hra není inicializována" << std::endl;
        return;
    }

    nlohmann::json gameState = serializeGameState();
    broadcastMessage(messageType::STATE, gameState.dump());

    std::cout << "✅ Stav hry odeslán" << std::endl;
}

void GameServer::sendGameStateToPlayer(int playerNumber) {
    std::cout << "📤 Posílám stav hry hráči #" << playerNumber << std::endl;

    std::lock_guard<std::mutex> lock(gameMutex);

    if (!game) {
        std::cerr << "⚠ Hra není inicializována" << std::endl;
        return;
    }

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

    // TODO: Získat aktivního hráče z game
    // int activePlayer = game->getActivePlayer();
    int activePlayer = 0; // Placeholder

    std::cout << "🔔 Notifikuji hráče #" << activePlayer << " že je na tahu" << std::endl;

    nlohmann::json turnData;
    turnData["message"] = "Je váš tah";
    turnData["playerNumber"] = activePlayer;

    sendToPlayer(activePlayer, messageType::YOUR_TURN, turnData.dump());

    std::cout << "✅ YOUR_TURN odesláno hráči #" << activePlayer << std::endl;
}