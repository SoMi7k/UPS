#include "NetworkManager.hpp"
#include "MessageHandler.hpp"
#include "GameManager.hpp"
#include "Protocol.hpp"
#include <iostream>

MessageHandler::MessageHandler(NetworkManager* networkManager, ClientManager* clientManager, GameManager* gameManager)
    : networkManager(networkManager),
      clientManager(clientManager),
      gameManager(gameManager) {

    std::cout << "📨 MessageHandler inicializován" << std::endl;
}

void MessageHandler::processClientMessage(ClientInfo* client, const Protocol::Message& msg) {

    std::cout << "\n📨 Od hráče #" << client->playerNumber << " ";

    Protocol::MessageType msgType = msg.type;
    std::vector<std::string> data = msg.fields;

    /*
    if (data.empty()) {
        std::cerr << "⚠ Nepodařilo se parsovat zprávu" << std::endl;
        sendError(client, Protocol::MessageType::ERROR, "Neplatný formát zprávy");
        return;
    }
    */

    std::cout << "🔄 Zpracovávám zprávu typu: " << static_cast<int>(msgType)
              << " od hráče #" << client->playerNumber << std::endl;

    // ===== HEARTBEAT =====
    if (msgType == Protocol::MessageType::HEARTBEAT) {
        handleHeartbeat(client);
    }
    // ===== TRICK =====
    else if (msgType == Protocol::MessageType::TRICK) {
        handleTrick(client);
    }
    // ===== CARD =====
    else if (msgType == Protocol::MessageType::CARD) {
        handleCard(data.at(0));
    }
    // ===== BIDDING =====
    else if (msgType == Protocol::MessageType::BIDDING) {
        handleBidding(data.at(0));
    }
    // ===== RESET =====
    else if (msgType == Protocol::MessageType::RESET) {
        handleReset(client, data.at(0));
    }
    // ===== DISCONNECT =====
    else if (msgType == Protocol::MessageType::DISCONNECT) {
        handleDisconnect(client);
    }
    // ===== CONNECT =====
    else if (msgType == Protocol::MessageType::CONNECT) {
        handleConnect(client);
    }
    // ===== UNKNOWN =====
    else {
        std::cerr << "⚠ Neznámý typ zprávy: " << static_cast<int>(msgType) << std::endl;
        sendError(client, Protocol::MessageType::DISCONNECT, "Neznámý typ zprávy: Odpojuji...\n");
        clientManager->disconnectClient(client);
    }
}

// ============================================================
// IMPLEMENTACE HANDLERŮ
// ============================================================

void MessageHandler::handleHeartbeat(ClientInfo* client) {
    std::cout << "💓 Heartbeat od hráče #" << client->playerNumber << std::endl;
    client->lastSeen = std::chrono::steady_clock::now();
}

void MessageHandler::handleTrick(ClientInfo* client) {
    gameManager->handleTrick(client);
}

void MessageHandler::handleCard(const std::string& data) {
    Card card = cardMapping(data);
    gameManager->handleCard(card);
}

void MessageHandler::handleBidding(std::string& label) {
    gameManager->handleBidding(label);

    std::this_thread::sleep_for(std::chrono::seconds(1));
    gameManager->notifyActivePlayer();
}

void MessageHandler::handleReset(ClientInfo* client, const std::string& data) {
    std::cout << "🔄 Hráč #" << client->playerNumber << " žádá o reset" << std::endl;

    if (data == "ANO") {
        clientManager->setreadyCount();
        clientManager->sendToPlayer(client->playerNumber, Protocol::MessageType::WAIT_LOBBY,
            {std::to_string(clientManager->getreadyCount())});
        std::cout << "  -> WAIT_LOBBY odesláno hráči #" << client->playerNumber << std::endl;
    } else {
        client->approved = false;
        networkManager->sendMessage(client->socket, client->playerNumber, Protocol::MessageType::DISCONNECT, {});
        clientManager->disconnectClient(client);
    }
}

void MessageHandler::handleDisconnect(ClientInfo* client) {
    std::cout << "👋 Hráč #" << client->playerNumber << " se odpojuje" << std::endl;
    clientManager->disconnectClient(client);
}

void MessageHandler::handleConnect(ClientInfo* client) {
    std::cout << "📨 Přijato CONNECT od hráče #" << client->playerNumber << std::endl;
}

void MessageHandler::sendError(ClientInfo* client, Protocol::MessageType msgType, const std::string& errorMessage) {
    std::string errorData = errorMessage.empty() ? "Chyba zpracování požadavku" : errorMessage;
    networkManager->sendMessage(client->socket, client->playerNumber, msgType, {errorData});
}