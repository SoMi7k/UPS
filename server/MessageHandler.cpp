#include "MessageHandler.hpp"
#include "NetworkManager.hpp"
#include "GameManager.hpp"
#include <iostream>

MessageHandler::MessageHandler(NetworkManager* networkManager, ClientManager* clientManager, GameManager* gameManager)
    : networkManager(networkManager),
      clientManager(clientManager),
      gameManager(gameManager) {

    std::cout << "📨 MessageHandler inicializován" << std::endl;
}

void MessageHandler::processClientMessage(ClientInfo* client, const std::string& message) {
    nlohmann::json msg = networkManager->deserialize(message);

    if (msg.empty()) {
        std::cerr << "⚠ Nepodařilo se parsovat zprávu" << std::endl;
        sendError(client, messageType::ERROR, "Neplatný formát zprávy");
        return;
    }

    std::string msgType = msg["type"];
    nlohmann::json data = msg.contains("data") ? msg["data"] : nlohmann::json{};

    std::cout << "🔄 Zpracovávám zprávu typu: " << msgType
              << " od hráče #" << client->playerNumber << std::endl;

    // ===== HEARTBEAT =====
    if (msgType == messageType::HEARTBEAT) {
        handleHeartbeat(client);
    }
    // ===== READY =====
    else if (msgType == messageType::READY) {
        handleReady(client);
    }
    // ===== TRICK =====
    else if (msgType == messageType::TRICK) {
        handleTrick(client);
    }
    // ===== CARD =====
    else if (msgType == messageType::CARD) {
        handleCard(client, data);
    }
    // ===== BIDDING =====
    else if (msgType == messageType::BIDDING) {
        handleBidding(data);
    }
    // ===== RESET =====
    else if (msgType == messageType::RESET) {
        handleReset(client, data);
    }
    // ===== DISCONNECT =====
    else if (msgType == messageType::DISCONNECT) {
        handleDisconnect(client);
    }
    // ===== CONNECT =====
    else if (msgType == messageType::CONNECT) {
        handleConnect(client);
    }
    // ===== UNKNOWN =====
    else {
        std::cerr << "⚠ Neznámý typ zprávy: " << msgType << std::endl;
        sendError(client, messageType::ERROR, "Neznámý typ zprávy: " + msgType);
    }
}

// ============================================================
// IMPLEMENTACE HANDLERŮ
// ============================================================

void MessageHandler::handleHeartbeat(ClientInfo* client) {
    std::cout << "💓 Heartbeat od hráče #" << client->playerNumber << std::endl;
    client->lastSeen = std::chrono::steady_clock::now();
}

void MessageHandler::handleReady(ClientInfo* client) {
    std::cout << "✅ Hráč #" << client->playerNumber << " je připraven" << std::endl;

    nlohmann::json okData;
    okData["message"] = "Připraven přijat";
    networkManager->sendMessage(client->socket, messageType::OK, okData.dump());
}

void MessageHandler::handleTrick(ClientInfo* client) {
    gameManager->handleTrick(client);
}

void MessageHandler::handleCard(ClientInfo* client, const nlohmann::json& data) {
    /*if (!data.contains("card")) {
        sendError(client, messageType::ERROR, "Chybí karta v požadavku");
        return;
    }*/

    Card card = cardMapping(data["card"]);
    gameManager->handleCard(client, card);
}

void MessageHandler::handleBidding(const nlohmann::json& data) {
    if (!data.contains("label")) {
        std::cerr << "❌ Chybí label v BIDDING zprávě" << std::endl;
        return;
    }

    std::string label = data["label"];
    gameManager->handleBidding(label);

    std::this_thread::sleep_for(std::chrono::seconds(1));
    gameManager->notifyActivePlayer();
}

void MessageHandler::handleReset(ClientInfo* client, const nlohmann::json& data) {
    std::cout << "🔄 Hráč #" << client->playerNumber << " žádá o reset" << std::endl;
    std::string reset = data["label"];

    if (reset == "ANO") {
        if (clientManager->getConnectedCount() < 2) {
            nlohmann::json waitData;
            waitData["current"] = clientManager->getConnectedCount();
            clientManager->sendToPlayer(client->playerNumber, messageType::WAIT_LOBBY, waitData.dump());
            std::cout << "  -> WAIT_LOBBY odesláno hráči #" << client->playerNumber << std::endl;
        } else {
            gameManager->startGame();
        }
    } else {
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

void MessageHandler::sendError(ClientInfo* client, const std::string& msgType, const std::string& errorMessage) {
    nlohmann::json errorData;
    errorData["message"] = errorMessage.empty() ? "Chyba zpracování požadavku" : errorMessage;
    networkManager->sendMessage(client->socket, msgType, errorData.dump());
}