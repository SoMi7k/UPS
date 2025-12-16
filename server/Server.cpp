#include "Server.hpp"
#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

// ============================================================
// KONSTRUKTOR A DESTRUKTOR
// ============================================================
GameServer::GameServer(const std::string& ip, int port, int requiredPlayers, int lobbies)
    : networkManager(std::make_unique<NetworkManager>(ip, port)),  // 🆕 Předáváme IP
      ip(ip),              // 🆕 Ukládáme IP
      port(port),
      running(false),
      requiredPlayers(requiredPlayers),
      lobbyCount(lobbies) {

    std::cout << "🔧 GameServer vytvořen" << std::endl;
    std::cout << "   - IP adresa: " << ip << std::endl;
    std::cout << "   - Port: " << port << std::endl;
    std::cout << "   - Počet hráčů: " << requiredPlayers << std::endl;
    std::cout << "   - Počet místností: " << lobbies << std::endl;
}


GameServer::~GameServer() {
    std::cout << "🗑️ GameServer destruktor - provádím cleanup..." << std::endl;
    if (running) {
        stop();
    }
    cleanup();
}

// ============================================================
// ACCEPT CLIENTS - Přijímání nových klientů
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

        // Najdeme volnou místnost
        Lobby* lobby = lobbyManager->findAvailableLobby();

        if (!lobby) {
            std::cout << "⚠ Všechny místnosti jsou plné, odmítám klienta" << std::endl;
            networkManager->sendMessage(clientSocket, -1, Protocol::MessageType::ERROR, {"All lobbies are full"});
            close(clientSocket);
            continue;
        }

        std::cout << "  -> Přiřazuji do Lobby #" << lobby->id << std::endl;

        // Přidáme klienta do místnosti
        ClientInfo* client = lobby->clientManager->addClient(clientSocket, clientIP);

        // Spuštění vlákna pro obsluhu klienta
        client->clientThread = std::thread(&GameServer::handleClient, this, client, lobby);
        client->clientThread.detach();

        std::cout << "✓ Vlákno pro hráče #" << client->playerNumber
                  << " (Lobby #" << lobby->id << ") spuštěno" << std::endl;

        // Zobrazíme status
        std::cout << lobbyManager->getLobbiesStatus();
    }
}

// ============================================================
// START GAME - Spuštění hry v samostatném vlákně
// ============================================================
void GameServer::startGame(Lobby* lobby) {
    while (running) {
        //std::cerr << "Počet připravených hráčů: " << lobby->clientManager->getreadyCount() << std::endl;
        if (lobby->clientManager->getreadyCount() == requiredPlayers && !lobby->gameStarted) {
            std::cout << "\n🎮 Lobby #" << lobby->id << " - Všichni hráči připojeni!" << std::endl;

            // ===== SPUŠTĚNÍ HRY =====
            std::cout << "\n🚀 Lobby #" << lobby->id << " - SPOUŠTÍM HRU!" << std::endl;
            lobby->gameManager->startGame();
            lobby->gameStarted = true;

            // Zobrazíme status
            std::cout << lobbyManager->getLobbiesStatus();
        }

        if (lobby->clientManager->getreadyCount() < requiredPlayers && lobby->gameStarted) {
                lobby->gameStarted = false;
                std::cout << "\n🚀 Lobby #" << lobby->id << " - Vypínám hru!" << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

// ============================================================
// HANDLE CLIENT - Spuštění threadu pro klienta
// ============================================================
void GameServer::handleClient(ClientInfo* client, Lobby* lobby) {
    if (client->playerNumber != -1) {
        std::cout << "\n>>> Vlákno pro hráče #" << client->playerNumber
                  << " (Lobby #" << lobby->id << ") zahájeno <<<" << std::endl;

        // ===== KROK 1: Poslání WELCOME zprávy =====
        std::vector<std::string> welcomeData;
        welcomeData.emplace_back(std::to_string(client->playerNumber));
        welcomeData.emplace_back(std::to_string(lobby->id));
        welcomeData.emplace_back(std::to_string(requiredPlayers));

        networkManager->sendMessage(client->socket, client->playerNumber,
                                   Protocol::MessageType::WELCOME, welcomeData);
    }

    // ===== KROK 2: Čekání na NICKNAME nebo RECONNECT =====
    std::vector<u_int8_t> initialMessage = networkManager->receiveMessage(client->socket);

    if (initialMessage.empty()) {
        std::cerr << "⚠ Hráč #" << client->playerNumber
                  << " se odpojil před odesláním zprávy (socket spadl)" << std::endl;
        lobby->clientManager->disconnectClient(client);
        return;
    }

    Protocol::Message msg = Protocol::deserialize(initialMessage);

    std::cout << "\n📨 Od hráče #" << client->playerNumber
              << " msgType: " << static_cast<int>(msg.type)
              << " (Lobby #" << lobby->id << "): ";

    for (size_t i = 0; i < msg.fields.size(); ++i) {
        std::cout << msg.fields[i];
        if (i + 1 < msg.fields.size()) {
            std::cout << " | ";
        }
    }
    std::cout << std::endl;

    if (msg.type != Protocol::MessageType::CONNECT && msg.type != Protocol::MessageType::RECONNECT) {
        std::cerr << "⚠ Hráč #" << client->playerNumber << " poslal nesprávný msgType" << std::endl;
        networkManager->sendMessage(client->socket, client->playerNumber,
                                       Protocol::MessageType::DISCONNECT, {"Nesprávný msgType"});
        lobby->clientManager->disconnectClient(client);
        return;
    }

    // Kontrola zda jde o RECONNECT
    if (msg.type == Protocol::MessageType::RECONNECT && !msg.fields[0].empty()) {
        std::string nickname = msg.fields[0];
        std::cout << "🔄 Pokus o reconnect se session ID: " << nickname << std::endl;

        ClientInfo* oldClient = lobby->clientManager->findDisconnectedClient(nickname);

        if (oldClient && lobby->clientManager->reconnectClient(oldClient, client->socket)) {
            std::cout << "✅ Hráč #" << oldClient->playerNumber << " úspěšně reconnectnut" << std::endl;
            int packetID = std::atoi(msg.fields[1].c_str());
            lobby->clientManager->sendLossPackets(oldClient, packetID);
            client = oldClient;
            networkManager->sendMessage(client->socket, client->playerNumber,
                                       Protocol::MessageType::RECONNECT, {});
        } else {
            std::cerr << "❌ Reconnect selhal" << std::endl;
            networkManager->sendMessage(client->socket, client->playerNumber,
                                       Protocol::MessageType::DISCONNECT,
                                       {"Reconnect failed - session expired or invalid"});
            lobby->clientManager->disconnectClient(client);
            return;
        }
    } else {
        // Běžný nový hráč
        std::string nickname = msg.fields[0];
        client->nickname = nickname;
        std::cout << "  -> Nickname přijat od hráče #" << client->playerNumber << std::endl;

        bool sameNickname = false;
        for (auto c : lobby->clientManager->getClients()) {
            if (c->nickname == nickname && c->playerNumber != client->playerNumber) {
                sameNickname = true;
            }
        }

        if (!sameNickname) {
            networkManager->sendMessage(client->socket, client->playerNumber,
                                       Protocol::MessageType::READY, {});
            std::cout << "  -> READY odesláno hráči #" << client->playerNumber << std::endl;
            client->approved = true;

            if (lobby->getConnectedCount() < requiredPlayers) {
                networkManager->sendMessage(client->socket, client->playerNumber,
                                          Protocol::MessageType::WAIT_LOBBY,
                                          {std::to_string(lobby->getConnectedCount())});
                std::cout << "  -> WAIT_LOBBY odesláno hráči #" << client->playerNumber << std::endl;
            }
        } else {
            std::cerr << "❌ Chyba: Stejné jméno!" << std::endl;
            networkManager->sendMessage(client->socket, client->playerNumber,
                                       Protocol::MessageType::DISCONNECT,
                                       {"Chyba: Stejné jméno!"});
            lobby->clientManager->disconnectClient(client);
            return;
        }
    }

    // ===== KROK 3: Hlavní smyčka =====
    std::cout << "  -> Vstupuji do příjmací smyčky pro hráče #" << client->playerNumber << std::endl;
    MessageHandler handler(networkManager.get(), lobby->clientManager.get(), lobby->gameManager.get());

    if (lobby->clientManager->getreadyCount() < requiredPlayers) {
        std::cout << "  -> Hráč #" << client->playerNumber << " oznámil připravenost" << std::endl;
        lobby->clientManager->setreadyCount();
    }

    // 🆕 HLAVNÍ PŘÍJMACÍ LOOP S DETEKCÍ ODPOJENÍ
    while (running && client->connected) {
        std::vector<u_int8_t> recvMsg = networkManager->receiveMessage(client->socket);

        // 🔴 DETEKCE ODPOJENÍ - prázdný buffer znamená spadlý socket
        if (recvMsg.empty()) {
            std::cout << "\n⚠ Hráč #" << client->playerNumber
                      << " ztratil spojení (socket spadl nebo byl zavřen)" << std::endl;
            lobby->clientManager->handleClientDisconnection(client);
            break;
        }

        // Aktualizace last seen (pouze pokud zpráva přišla)
        client->lastSeen = std::chrono::steady_clock::now();

        try {
            handler.processClientMessage(client, recvMsg);
        } catch (const std::exception& e) {
            std::cerr << "❌ Výjimka při zpracování zprávy: " << e.what() << std::endl;
            networkManager->sendMessage(client->socket, client->playerNumber,
                                       Protocol::MessageType::DISCONNECT,
                                       {"Interní chyba serveru"});
            break;
        }
    }

    std::cout << "\n<<< Vlákno pro hráče #" << client->playerNumber
              << " (Lobby #" << lobby->id << ") končí >>>" << std::endl;
}

// ============================================================
// HLAVNÍ METODY - START, STOP
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

    // Vytvoření místností (musí být až po inicializaci socketu)
    lobbyManager = std::make_unique<LobbyManager>(networkManager.get(), requiredPlayers, lobbyCount);

    running = true;

    // Spuštění vláken pro každou lobby
    for (int i = 1; i <= lobbyCount; i++) {
        Lobby* lobby = lobbyManager->getLobby(i);
        if (lobby) {
            std::thread gameThread(&GameServer::startGame, this, lobby);
            gameThread.detach();
            std::cout << "🎲 Spuštěno game-thread pro Lobby #" << lobby->id << std::endl;
        }
    }

    // Spuštění accept threadu
    std::cout << "\n🔄 Spouštím accept thread..." << std::endl;
    acceptThread = std::thread(&GameServer::acceptClients, this);

    // Spuštění timeout checkeru pro všechny místnosti
    std::thread timeoutThread([this]() {
        std::cout << "🕒 Spouštím timeout checker..." << std::endl;

        while (running) {
            for (int i = 1; i <= lobbyCount; i++) {
                Lobby* lobby = lobbyManager->getLobby(i);
                if (lobby && lobby->clientManager) {
                    lobby->clientManager->checkDisconnectedClients(running);
                }
            }

            std::this_thread::sleep_for(std::chrono::seconds(2));
        }

        std::cout << "🛑 Timeout checker zastaven" << std::endl;
    });
    timeoutThread.detach();

    std::cout << "\n✅ Server úspěšně spuštěn!" << std::endl;
    std::cout << "📡 Naslouchám na portu " << port << std::endl;
    std::cout << "🏠 Počet místností: " << lobbyCount << std::endl;
    std::cout << "⏳ Každá místnost čeká na " << requiredPlayers << " hráče..." << std::endl;
    std::cout << lobbyManager->getLobbiesStatus();
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

    // Odpojení všech klientů ze všech místností
    if (lobbyManager) {
        lobbyManager->disconnectAll();
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

std::string GameServer::getStatus() const {
    if (lobbyManager) {
        return lobbyManager->getLobbiesStatus();
    }
    return "Server není inicializován";
}

// ============================================================
// CLEANUP - Úklid zdrojů
// ============================================================
void GameServer::cleanup() {
    std::cout << "🧹 Provádím cleanup..." << std::endl;

    if (lobbyManager) {
        lobbyManager.reset();
    }

    if (networkManager) {
        networkManager.reset();
    }

    std::cout << "✅ Cleanup dokončen" << std::endl;
}