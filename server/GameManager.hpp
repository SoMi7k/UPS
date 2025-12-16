#ifndef GAME_MANAGER_HPP
#define GAME_MANAGER_HPP

#include "ClientManager.hpp"
#include "Protocol.hpp"
#include "game/Game.hpp"
#include <condition_variable>
#include <mutex>

class ClientManager;
class NetworkManager;

class GameManager {
public:
    GameManager(int requiredPlayers, NetworkManager* networkManager, ClientManager* clientManager);
    ~GameManager();

    void startGame();
    void initPlayers();

    // ============================================================
    // PRIVÁTNÍ METODY - Serializace
    // ============================================================
    std::vector<std::string> serializeGameStart(int playerNumber);
    std::vector<std::string> serializeGameState();
    std::string serializePlayer(int playerNumber);
    std::vector<std::string> serializeInvalid(int playerNumber);

    // ============================================================
    // PRIVÁTNÍ METODY - Herní logika
    // ============================================================
    void sendGameStateToPlayer(int playerNumber);
    void sendInvalidPlayer(int playerNumber);
    void notifyActivePlayer();

    // ============================================================
    // PRIVÁTNÍ METODY - Handlery
    // ============================================================
    void handleTrick(ClientInfo* client);
    void handleBidding(std::string& label);
    void handleCard(Card card);

private:
    std::unique_ptr<NetworkManager> networkManager;
    std::unique_ptr<ClientManager> clientManager;
    int requiredPlayers;

    int free_number = 0;
    std::unique_ptr<Game> game;    // Instance hry
    std::mutex gameMutex;          // Mutex pro thread-safe přístup ke hře
    std::mutex trickMutex;
    std::condition_variable trickCV;
    int trickResponses = 0;
};

#endif