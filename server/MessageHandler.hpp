#ifndef MESSAGE_HANDLER_HPP
#define MESSAGE_HANDLER_HPP

#include <string>
#include "GameManager.hpp"

class GameManager;
class NetworkManager;

class MessageHandler {
public:
    MessageHandler(NetworkManager* networkManager, ClientManager* clientManager, GameManager* gameManager);

    // Zpracování zpráv
    void processClientMessage(ClientInfo* client, const Protocol::Message& msg);

private:
    NetworkManager* networkManager;
    ClientManager* clientManager;
    GameManager* gameManager;

    // Jednotlivé handlery pro různé typy zpráv
    void handleTrick(ClientInfo* client);
    void handleCard(const std::string&);
    void handleBidding(std::string& data);
    void handleReset(ClientInfo* client, const std::string& data);
    void handleDisconnect(ClientInfo* client);
    void handleConnect(ClientInfo* client);

    // Pomocné funkce
    void sendError(ClientInfo* client, Protocol::MessageType, const std::string& errorMessage);
};

#endif // MESSAGE_HANDLER_HPP