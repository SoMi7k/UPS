#ifndef LOBBYMANAGER_HPP
#define LOBBYMANAGER_HPP

#include <memory>
#include <vector>
#include <mutex>
#include <string>
#include <condition_variable>

// Forward deklarace
class NetworkManager;
class ClientManager;
class GameManager;

struct Lobby {
    int id;
    int requiredPlayers;
    std::unique_ptr<ClientManager> clientManager;
    std::unique_ptr<GameManager> gameManager;
    bool gameStarted;

    // Synchronizační bariéra
    std::mutex readyMutex;
    std::condition_variable readyCV;

    Lobby(int lobbyId, int players, NetworkManager* netManager);
    ~Lobby();

    int getConnectedCount() const;
    int getActiveCount() const;
    bool isFull() const;
    bool canJoin() const;

    // Metody pro bariéru
    void playerReady();
    void waitForAllPlayers();
    void resetBarrier();
};

class LobbyManager {
private:
    std::vector<std::unique_ptr<Lobby>> lobbies;
    std::mutex lobbiesMutex;
    NetworkManager* networkManager;
    int requiredPlayers;

public:
    LobbyManager(NetworkManager* netManager, int players, int lobbyCount);
    ~LobbyManager();

    // Najde volnou místnost pro nového hráče
    Lobby* findAvailableLobby();

    // Najde místnost podle ID
    Lobby* getLobby(int lobbyId);

    // Získá statistiky všech místností
    std::string getLobbiesStatus();

    // Počet místností
    int getLobbyCount() const { return lobbies.size(); }

    // Odpojí všechny klienty ze všech místností
    void disconnectAll();
};

#endif // LOBBYMANAGER_HPP