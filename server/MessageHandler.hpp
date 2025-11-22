#ifndef MESSAGE_HANDLER_HPP
#define MESSAGE_HANDLER_HPP

#include <string>
#include <nlohmann/json.hpp>
#include "GameManager.hpp"

class GameManager;
class NetworkManager;

class MessageHandler {
public:
    MessageHandler(NetworkManager* networkManager, ClientManager* clientManager, GameManager* gameManager);

    // Zpracování zpráv
    void processClientMessage(ClientInfo* client, const std::string& message);

private:
    NetworkManager* networkManager;
    ClientManager* clientManager;
    GameManager* gameManager;

    // Jednotlivé handlery pro různé typy zpráv
    void handleHeartbeat(ClientInfo* client);
    void handleReady(ClientInfo* client, const nlohmann::json& data);
    void handleTrick(ClientInfo* client);
    void handleCard(ClientInfo* client, const nlohmann::json& data);
    void handleBidding(const nlohmann::json& data);
    void handleReset(ClientInfo* client, const nlohmann::json& data);
    void handleDisconnect(ClientInfo* client);
    void handleConnect(ClientInfo* client, const nlohmann::json& data);

    // Pomocné funkce
    void sendError(ClientInfo* client, const std::string& msgType, const std::string& errorMessage);
};

#endif // MESSAGE_HANDLER_HPP