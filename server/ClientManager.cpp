#include "ClientManager.hpp"
#include "NetworkManager.hpp"
#include <iostream>
#include <algorithm>
#include <unistd.h>
#include <sys/socket.h>

ClientManager::ClientManager(int requiredPlayers, NetworkManager* networkManager)
    : requiredPlayers(requiredPlayers), connectedPlayers(0), networkManager(networkManager) {
    std::cout << "🔧 ClientManager vytvořen (požadováno " << requiredPlayers << " hráčů)" << std::endl;
}

ClientManager::~ClientManager() {
    std::lock_guard<std::mutex> lock(clientsMutex);

    for (auto* client : clients) {
        if (client) {
            if (client->clientThread.joinable()) {
                client->clientThread.detach();
            }
            if (client->socket >= 0) {
                close(client->socket);
            }
            delete client;
        }
    }
    clients.clear();
}

ClientInfo* ClientManager::addClient(int socket, const std::string& address) {
    std::lock_guard<std::mutex> lock(clientsMutex);

    auto* client = new ClientInfo{
        socket,
        connectedPlayers,
        address,
        true,
        std::thread(),
        generateSessionId(),
        std::chrono::steady_clock::now(),
        false,
        ""
    };

    clients.push_back(client);
    connectedPlayers++;

    std::cout << "✓ Klient #" << client->playerNumber << " přidán (celkem: "
              << connectedPlayers << "/" << requiredPlayers << ")" << std::endl;

    return client;
}

void ClientManager::removeClient(ClientInfo* client) {
    if (!client) return;

    std::lock_guard<std::mutex> lock(clientsMutex);

    auto it = std::find(clients.begin(), clients.end(), client);
    if (it != clients.end()) {
        clients.erase(it);
        connectedPlayers--;
        std::cout << "✓ Klient #" << client->playerNumber << " odstraněn" << std::endl;
    }
}

void ClientManager::disconnectAll() {
    std::vector<ClientInfo*> clientsCopy;
    {
        std::lock_guard<std::mutex> lock(clientsMutex);
        clientsCopy = clients;
        std::cout << "🔌 Odpojuji " << clientsCopy.size() << " klientů..." << std::endl;
    }

    for (auto* client : clientsCopy) {
        if (client && client->connected) {
            networkManager->sendMessage(client->socket, messageType::DISCONNECT, "Server se vypíná");
            shutdown(client->socket, SHUT_RDWR);
            close(client->socket);
            client->connected = false;
        }
    }
}

void ClientManager::disconnectClient(ClientInfo* client) {
    if (!client) return;

    std::cout << "\n" << std::string(50, '-') << std::endl;
    std::cout << "🔌 Odpojuji hráče #" << client->playerNumber << std::endl;
    std::cout << "  - IP: " << client->address << std::endl;
    std::cout << "  - Socket: " << client->socket << std::endl;

    client->connected = false;

    if (client->socket >= 0) {
        shutdown(client->socket, SHUT_RDWR);
        close(client->socket);
        client->socket = -1;
    }

    {
        std::lock_guard<std::mutex> lock(clientsMutex);
        auto it = std::find(clients.begin(), clients.end(), client);
        if (it != clients.end()) {
            clients.erase(it);
            connectedPlayers--;
            std::cout << "  - Odstraněn ze seznamu" << std::endl;
            std::cout << "  - Zbývá " << connectedPlayers << "/" << requiredPlayers << " hráčů" << std::endl;
        }
    }

    // Notifikace ostatních - teď je bezpečná
    nlohmann::json statusData;
    statusData["message"] = "Hráč se odpojil";
    statusData["playerNumber"] = client->playerNumber;
    statusData["connectedPlayers"] = connectedPlayers;
    broadcastMessage(messageType::STATUS, statusData.dump());

    std::cout << "✅ Hráč #" << client->playerNumber << " odpojen" << std::endl;
    std::cout << std::string(50, '-') << std::endl;
}

ClientInfo* ClientManager::findClientBySocket(int socket) {
    std::lock_guard<std::mutex> lock(clientsMutex);

    for (auto* client : clients) {
        if (client && client->socket == socket) {
            return client;
        }
    }
    return nullptr;
}

ClientInfo* ClientManager::findClientByPlayerNumber(int playerNumber) {
    std::lock_guard<std::mutex> lock(clientsMutex);

    for (auto* client : clients) {
        if (client && client->playerNumber == playerNumber && client->connected) {
            return client;
        }
    }
    return nullptr;
}

ClientInfo* ClientManager::findDisconnectedClient(const std::string& sessionId) {
    std::lock_guard<std::mutex> lock(clientsMutex);

    for (auto* client : clients) {
        if (client && client->isDisconnected && client->sessionId == sessionId) {
            auto elapsed = std::chrono::steady_clock::now() - client->lastSeen;
            auto seconds = std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();

            if (seconds < RECONNECT_TIMEOUT_SECONDS) {
                return client;
            }
        }
    }
    return nullptr;
}

bool ClientManager::reconnectClient(ClientInfo* oldClient, int newSocket) {
    if (!oldClient) return false;

    std::cout << "🔄 Reconnecting hráče #" << oldClient->playerNumber << std::endl;

    if (oldClient->socket >= 0) {
        close(oldClient->socket);
    }

    oldClient->socket = newSocket;
    oldClient->connected = true;
    oldClient->isDisconnected = false;
    oldClient->lastSeen = std::chrono::steady_clock::now();

    nlohmann::json statusData;
    statusData["message"] = "Hráč se znovu připojil";
    statusData["playerNumber"] = oldClient->playerNumber;
    statusData["nickname"] = oldClient->nickname;
    broadcastMessage(messageType::STATUS, statusData.dump());

    return true;
}

void ClientManager::handleClientDisconnection(ClientInfo* client) {
    if (!client) return;

    std::cout << "\n🔌 Hráč #" << client->playerNumber << " se odpojil - čekám na reconnect" << std::endl;

    client->connected = false;
    client->isDisconnected = true;
    client->lastSeen = std::chrono::steady_clock::now();

    if (client->socket >= 0) {
        shutdown(client->socket, SHUT_RDWR);
        close(client->socket);
        client->socket = -1;
    }

    nlohmann::json statusData;
    statusData["message"] = "Hráč ztratil spojení - čekáme na reconnect";
    statusData["playerNumber"] = client->playerNumber;
    statusData["reconnectTimeout"] = RECONNECT_TIMEOUT_SECONDS;
    broadcastMessage(messageType::STATUS, statusData.dump());

    std::cout << "⏳ Čekám " << RECONNECT_TIMEOUT_SECONDS << "s na reconnect hráče #"
              << client->playerNumber << std::endl;
}

void ClientManager::checkDisconnectedClients(bool& running) {
    while (running) {
        std::this_thread::sleep_for(std::chrono::seconds(10));

        std::vector<ClientInfo*> toRemove;

        {
            std::lock_guard<std::mutex> lock(clientsMutex);
            auto now = std::chrono::steady_clock::now();

            for (auto* client : clients) {
                if (client && client->isDisconnected) {
                    auto elapsed = now - client->lastSeen;
                    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();

                    if (seconds >= RECONNECT_TIMEOUT_SECONDS) {
                        std::cout << "⏱️ Timeout pro hráče #" << client->playerNumber
                                  << " - odstraňuji permanentně" << std::endl;
                        toRemove.push_back(client);
                    }
                }
            }
        }

        for (auto* client : toRemove) {
            disconnectClient(client);
        }
    }
}

int ClientManager::getConnectedCount() const {
    return connectedPlayers;
}

int ClientManager::getActiveCount() const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(clientsMutex));

    int count = 0;
    for (auto* c : clients) {
        if (c && !c->isDisconnected) {
            count++;
        }
    }
    return count;
}

std::vector<ClientInfo*> ClientManager::getClients() {
    std::lock_guard<std::mutex> lock(clientsMutex);
    return clients;
}

void ClientManager::broadcastMessage(const std::string& msgType, const std::string& message) {
    std::lock_guard<std::mutex> lock(clientsMutex);
    std::cout << "📢 Broadcast: " << msgType << std::endl;

    for (auto* client : clients) {
        if (client && client->connected) {
            networkManager->sendMessage(client->socket, msgType, message);
        }
    }
}

void ClientManager::sendToPlayer(int playerNumber, const std::string& msgType, const std::string& message) {
    std::lock_guard<std::mutex> lock(clientsMutex);

    for (auto* client : clients) {
        if (client && client->playerNumber == playerNumber && client->connected) {
            networkManager->sendMessage(client->socket, msgType, message);
            return;
        }
    }

    std::cerr << "⚠ Hráč #" << playerNumber << " nebyl nalezen" << std::endl;
}

std::string ClientManager::generateSessionId() {
    static int counter = 0;
    auto now = std::chrono::system_clock::now().time_since_epoch().count();
    return "SESSION_" + std::to_string(now) + "_" + std::to_string(counter++);
}