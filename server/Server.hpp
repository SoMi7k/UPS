#ifndef SERVER_HPP
#define SERVER_HPP

#include "ClientManager.hpp"
#include "NetworkManager.hpp"
#include "GameManager.hpp"
#include "MessageHandler.hpp"
#include <thread>
#include <memory>

// Hlavní třída serveru
class GameServer {
private:
    std::unique_ptr<NetworkManager> networkManager;
    std::unique_ptr<ClientManager> clientManager;
    std::unique_ptr<GameManager> gameManager;
    std::unique_ptr<MessageHandler> messageHandler;;

    // ============================================================
    // SÍŤOVÉ NASTAVENÍ
    // ============================================================
    int port;
    bool running;                  // Zda server běží
    std::thread acceptThread;      // Vlákno pro přijímání klientů
    int requiredPlayers;

    // ============================================================
    // PRIVÁTNÍ METODY - Síťové operace
    // ============================================================
    void acceptClients();
    void handleClient(ClientInfo* client);

    // ============================================================
    // PRIVÁTNÍ METODY - Pomocné funkce
    // ============================================================
    void disconnectClient(ClientInfo* client);
    void cleanup();

public:
    // ============================================================
    // VEŘEJNÉ METODY
    // ============================================================
    GameServer(int port = 10000, int requiredPlayers = 2);
    ~GameServer();

    void start();
    void stop();
    bool isRunning() const;
};

#endif