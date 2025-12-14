#ifndef SERVER_HPP
#define SERVER_HPP

#include "NetworkManager.hpp"
#include "ClientManager.hpp"
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

    std::string ip;
    int port;
    std::atomic<bool> running;
    int requiredPlayers;
    int lobbyCount;
    std::thread acceptThread;

    void startGame(Lobby* lobby);
    void acceptClients();
    void handleClient(ClientInfo* client, Lobby* lobby);
    void cleanup();

public:
    // 🆕 Konstruktor s IP adresou
    GameServer(const std::string& ip, int port, int requiredPlayers, int lobbies);
    ~GameServer();

    void start();
    void stop();
    bool isRunning() const;

    std::string getStatus() const;
};

#endif // SERVER_HPP