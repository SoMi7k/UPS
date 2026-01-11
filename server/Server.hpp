#ifndef SERVER_HPP
#define SERVER_HPP

#include "ClientManager.hpp"
#include "LobbyManager.hpp"
#include "MessageHandler.hpp"
#include "NetworkManager.hpp"
#include <atomic>
#include <memory>
#include <optional>
#include <thread>


class GameServer {
private:
  std::unique_ptr<NetworkManager> networkManager;
  std::unique_ptr<LobbyManager> lobbyManager;
  std::unique_ptr<MessageHandler> messageHandler;

  std::string ip;            // IP adresa
  int port;                  // Port
  std::atomic<bool> running; // Příznak běhu serveru
  int requiredPlayers;       // Požadovaný počet hráčů
  int lobbyCount;            // Počet lobby
  std::thread acceptThread;  // Vlákno pro připojení klientů

  void startGame(Lobby *lobby);
  void acceptClients();
  void handleClient(ClientInfo *client, Lobby *lobby);
  void cleanup();

public:
  // 🆕 Konstruktor s IP adresou
  GameServer(const std::string &ip, int port, int requiredPlayers, int lobbies);
  ~GameServer();

  void start();
  void stop();
  bool isRunning() const;
  std::string getStatus() const;

  std::optional<Protocol::Message>
  msgValidation(Lobby *lobby, ClientInfo *client, const std::string &recvMsg);
};

#endif // SERVER_HPP