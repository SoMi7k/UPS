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

    // Serializace
    std::vector<std::string> serializeGameStart(int playerNumber);
    std::vector<std::string> serializeGameState();
    std::string serializePlayer(int playerNumber);
    std::vector<std::string> serializeInvalid(int playerNumber);

    // Herní logika
    void sendGameStateToPlayer(int playerNumber);
    void sendInvalidPlayer(int playerNumber);
    void notifyActivePlayer();

    // Handlery
    void handleTrick(ClientInfo* client);
    void handleBidding(std::string& label);
    void handleCard(Card card);

private:
    std::unique_ptr<NetworkManager> networkManager;
    std::unique_ptr<ClientManager> clientManager;
    int requiredPlayers;             // Požadovaný počet hráčů
    std::unique_ptr<Game> game;      // Instance hry
    std::mutex gameMutex;            // Mutex pro thread-safe přístup ke hře
    std::mutex trickMutex;           // Mutex pro thread-safe přístup ke štychu
    std::condition_variable trickCV; // Podmíková promměná pro další štych
    int trickResponses = 0;          // Počet hráčů připravených na další štych
};

#endif