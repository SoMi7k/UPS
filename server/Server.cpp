#include "server.hpp"
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <chrono>
#include <string>
#include <net/if.h>
#include <vector>

#define QUEUE_LENGTH 10

// ============================================================
// KONSTRUKTOR A DESTRUKTOR
// ============================================================

GameServer::GameServer(int port, int requiredPlayers)
    : networkManager(std::make_unique<NetworkManager>(port)),
      clientManager(std::make_unique<ClientManager>(requiredPlayers, networkManager.get())),
      gameManager(std::make_unique<GameManager>(requiredPlayers, networkManager.get(), clientManager.get())),
      messageHandler(std::make_unique<MessageHandler>(networkManager.get(), clientManager.get(),
          gameManager.get())),
      port(port),
      running(false),
      requiredPlayers(requiredPlayers) {

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
// 2. ACCEPT CLIENTS - Přijímání nových klientů
// ============================================================

void GameServer::acceptClients() {
    std::cout << "\n=== Čekám na připojení klientů ===" << std::endl;

    while (running) {
        sockaddr_in clientAddress{};
        socklen_t clientLen = sizeof(clientAddress);

        int clientSocket = accept(networkManager->getServerSocket(),
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

        // OPRAVENO: Použijeme getActiveCount() místo lockClientMutex()
        int activeConnections = clientManager->getActiveCount();

        if (activeConnections >= requiredPlayers) {
            std::cout << "⚠ Hra je plná, odmítám klienta" << std::endl;
            networkManager->sendMessage(clientSocket, messageType::ERROR, "Game is full");
            close(clientSocket);
            continue;
        }

        ClientInfo* client = clientManager->addClient(clientSocket, clientIP);

        // Spuštění vlákna
        client->clientThread = std::thread(&GameServer::handleClient, this, client);
        client->clientThread.detach();

        std::cout << "✓ Vlákno pro hráče #" << client->playerNumber << " spuštěno" << std::endl;

        // Pokud máme všechny hráče, spustíme hru
        if (clientManager->getConnectedCount() == requiredPlayers) {
            std::cout << "\n🎮 Všichni hráči připojeni - spouštím hru!" << std::endl;
            gameManager->startGame();
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
    networkManager->sendMessage(client->socket, messageType::WELCOME, welcomeData.dump());

    // ===== KROK 2: Čekání na NICKNAME nebo RECONNECT =====
    std::string initialMessage = networkManager->receiveMessage(client->socket);
    if (initialMessage.empty()) {
        std::cerr << "⚠ Hráč #" << client->playerNumber << " se odpojil před odesláním zprávy" << std::endl;
        clientManager->handleClientDisconnection(client);
        return;
    }

    nlohmann::json msgJson = networkManager->deserialize(initialMessage);
    if (msgJson.empty()) {
        clientManager->handleClientDisconnection(client);
        return;
    }
    std::string msgType = msgJson["type"];

    // Kontrola zda jde o RECONNECT
    if (msgType == messageType::CONNECT && msgJson["data"].contains("sessionId")) {
        std::string sessionId = msgJson["data"]["sessionId"];
        std::cout << "🔄 Pokus o reconnect se session ID: " << sessionId << std::endl;

        ClientInfo* oldClient = clientManager->findDisconnectedClient(sessionId);
        if (oldClient && clientManager->reconnectClient(oldClient, client->socket)) {
            std::cout << "✅ Hráč #" << oldClient->playerNumber << " úspěšně reconnectnut" << std::endl;

            // Pošleme aktuální stav hry
            gameManager->sendGameStateToPlayer(oldClient->playerNumber);
            nlohmann::json clientData;
            clientData["client"] = gameManager->serializePlayer(oldClient->playerNumber);
            clientManager->sendToPlayer(oldClient->playerNumber, messageType::CLIENT_DATA, clientData.dump());

            // Pokračujeme se starým clientem
            client = oldClient;
        } else {
            std::cerr << "❌ Reconnect selhal" << std::endl;
            nlohmann::json errorData;
            errorData["message"] = "Reconnect failed - session expired or invalid";
            networkManager->sendMessage(client->socket, messageType::ERROR, errorData.dump());
            disconnectClient(client);
            return;
        }
    } else {
        // Běžný nový hráč
        std::string nickname = msgJson["data"]["nickname"];
        client->nickname = nickname;
        gameManager->initPlayer(client, nickname);

        std::cout << "  -> Nickname přijat od hráče #" << client->playerNumber << std::endl;

        nlohmann::json waitData;
        waitData["current"] = clientManager->getConnectedCount();

        if (clientManager->getConnectedCount() < requiredPlayers) {
            networkManager->sendMessage(client->socket, messageType::WAIT_LOBBY, waitData.dump());
            std::cout << "  -> WAIT_LOBBY odesláno hráči #" << client->playerNumber << std::endl;
        }
    }

    // ===== KROK 3: Hlavní smyčka =====
    std::cout << "  -> Vstupuji do příjmací smyčky pro hráče #" << client->playerNumber << std::endl;

    while (running && client->connected) {
        std::string message = networkManager->receiveMessage(client->socket);
        if (message.empty()) {
            disconnectClient(client);
            return;
        }

        if (message.empty()) {
            std::cout << "\n⚠ Hráč #" << client->playerNumber << " ztratil spojení" << std::endl;
            clientManager->handleClientDisconnection(client);
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
            messageHandler->processClientMessage(client, message);
        } catch (const std::exception& e) {
            std::cerr << "❌ Výjimka při zpracování zprávy: " << e.what() << std::endl;
            nlohmann::json errorData;
            errorData["message"] = "Interní chyba serveru";
            networkManager->sendMessage(client->socket, messageType::ERROR, errorData.dump());
        }
    }

    std::cout << "\n<<< Vlákno pro hráče #" << client->playerNumber << " končí >>>" << std::endl;
}

// ============================================================
// HLAVNÍ METODY - START, STOP, IS RUNNING
// ============================================================

void GameServer::start() {
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "🚀 SPOUŠTÍM SERVER" << std::endl;
    std::cout << std::string(60, '=') << std::endl;

    // Inicializace socketu
    if (!networkManager->initializeSocket()) {
        std::cerr << "❌ Nepodařilo se inicializovat socket" << std::endl;
        return;
    }

    running = true;

    // Spuštění accept threadu
    std::cout << "\n🔄 Spouštím accept thread..." << std::endl;
    acceptThread = std::thread(&GameServer::acceptClients, this);

    // Spuštění timeout checkeru
    std::thread timeoutThread([this]() {
        clientManager->checkDisconnectedClients(running);
    });
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
    networkManager->closeServerSocket();

    // Odpojení všech klientů
    clientManager->disconnectAll();

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
// DISCONNECT CLIENT - Odpojení jednoho klienta
// ============================================================

void GameServer::disconnectClient(ClientInfo* client) {
    clientManager->disconnectClient(client);
}

// ============================================================
// CLEANUP - Úklid zdrojů
// ============================================================

void GameServer::cleanup() {
    std::cout << "🧹 Provádím cleanup..." << std::endl;

    if (messageHandler) {
        messageHandler.reset();
    }

    if (gameManager) {
        gameManager.reset();
    }

    if (clientManager) {
        clientManager.reset();
    }

    if (networkManager) {
        networkManager.reset();
    }

    std::cout << "✅ Cleanup dokončen" << std::endl;
}