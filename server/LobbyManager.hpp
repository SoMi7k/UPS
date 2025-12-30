#ifndef LOBBYMANAGER_HPP
#define LOBBYMANAGER_HPP

#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
#include <vector>


// Forward deklarace
class NetworkManager;
class ClientManager;
class GameManager;

struct Lobby {
  std::unique_ptr<ClientManager> clientManager;
  std::unique_ptr<GameManager> gameManager;
  int id;              // ID místnosti
  bool gameStarted;    // Příznak pro začátek hry
  int requiredPlayers; // Počet požadovaných hráčů

  Lobby(int lobbyId, int players, NetworkManager *netManager);
  ~Lobby();

  int getConnectedCount() const; // Vrátí počet připojených hráčů v lobby
  int getActiveCount() const;    // Vrátí počet aktivních hráčů
  bool isFull() const;           // Zjistí zda je lobby plně obsazené
  bool canJoin() const;          // Příznak zda se může klient připojit do lobby
};

class LobbyManager {
private:
  NetworkManager *networkManager;
  int requiredPlayers;                         // Počet požadovaných hráčů
  std::vector<std::unique_ptr<Lobby>> lobbies; // Pole místností
  std::mutex lobbiesMutex;                     // Mutex pro přístup do místností

public:
  LobbyManager(NetworkManager *netManager, int players, int lobbyCount);
  ~LobbyManager();
  Lobby *findAvailableLobby();    // Najde volnou místnost pro nového hráče
  Lobby *getLobby(int lobbyId);   // Najde místnost podle ID
  std::string getLobbiesStatus(); // Získá statistiky všech místností
  int getLobbyCount() const { return lobbies.size(); } // Počet místností
  void disconnectAll(); // Odpojí všechny klienty ze všech místností
};

#endif // LOBBYMANAGER_HPP