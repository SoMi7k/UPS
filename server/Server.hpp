#ifndef SERVER_HPP
#define SERVER_HPP

#include "NetworkManager.hpp"
#include "ClientManager.hpp"
#include "GameManager.hpp"
#include "MessageHandler.hpp"
#include "LobbyManager.hpp"
#include <memory>
#include <thread>
#include <atomic>

class GameServer {
private:
    std::unique_ptr<NetworkManager> networkManager;
    std::unique_ptr<LobbyManager> lobbyManager;
    std::unique_ptr<MessageHandler> messageHandler;

    int port;
    std::atomic<bool> running;
    int requiredPlayers;
    int lobbyCount;
    std::thread acceptThread;

    void acceptClients();
    void handleClient(ClientInfo* client, Lobby* lobby);
    void cleanup();

public:
    GameServer(int port, int requiredPlayers = 2, int lobbies = 1);
    ~GameServer();

    void start();
    void stop();
    bool isRunning() const;

    std::string getStatus() const;
};

#endif // SERVER_HPP