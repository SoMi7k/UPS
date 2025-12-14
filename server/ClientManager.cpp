#include "ClientManager.hpp"
#include "NetworkManager.hpp"
#include <iostream>
#include <algorithm>
#include <unistd.h>
#include <sys/socket.h>

ClientManager::ClientManager(int requiredPlayers, NetworkManager* networkManager)
    : requiredPlayers(requiredPlayers), connectedPlayers(0), networkManager(networkManager) {
    std::cout << "🔧 ClientManager vytvořen (požadováno " << requiredPlayers << " hráčů)" << std::endl;
    clientNumbers.resize(requiredPlayers, 0);
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

int ClientManager::getFreeNumber() {
    for (int i = 0; i < requiredPlayers; i++) {
        if (clientNumbers[i] == 0) {
            clientNumbers[i] = 1;
            return i;
        }
    }

    return -1;
}

ClientInfo* ClientManager::addClient(int socket, const std::string& address) {
    std::lock_guard<std::mutex> lock(clientsMutex);

    auto* client = new ClientInfo{
        socket,
        getFreeNumber(),
        address,
        true,
        std::thread(),
        std::chrono::steady_clock::now(),
        false,
        "",
        false
    };

    connectedPlayers++;
    clients.push_back(client);

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
            networkManager->sendMessage(client->socket, client->playerNumber, messageType::DISCONNECT, "Server se vypíná");
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
    clientNumbers[client->playerNumber] = 0;

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

    if (client->approved) {
        readyCount--;
    }

    // Notifikace ostatních - teď je bezpečná
    nlohmann::json statusData;
    statusData["code"] = 1;
    statusData["nickname"] = client->nickname;
    statusData["connectedPlayers"] = connectedPlayers;

    if (client->approved) {
        for (auto c : clients) {
            if (c->playerNumber != client->playerNumber) {
                sendToPlayer(c->playerNumber, messageType::STATUS, statusData.dump());
            }
        }
    }

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

ClientInfo* ClientManager::findDisconnectedClient(const std::string& nickname) {
    std::lock_guard<std::mutex> lock(clientsMutex);

    for (auto* client : clients) {
        if (client && client->isDisconnected && client->nickname == nickname) {
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
    statusData["code"] = 3;
    statusData["nickname"] = oldClient->nickname;

    for (auto c : clients) {
        if (c->playerNumber != oldClient->playerNumber) {
            sendToPlayer(c->playerNumber, messageType::STATUS, statusData.dump());
        }
    }

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

    connectedPlayers--;

    nlohmann::json statusData;
    statusData["code"] = 2;
    statusData["nickname"] = client->nickname;
    statusData["reconnectTimeout"] = RECONNECT_TIMEOUT_SECONDS;

    for (auto c : clients) {
        if (c->playerNumber != client->playerNumber) {
            sendToPlayer(c->playerNumber, messageType::STATUS, statusData.dump());
        }
    }

    std::cout << "⏳ Čekám " << RECONNECT_TIMEOUT_SECONDS << "s na reconnect hráče #"
              << client->playerNumber << std::endl;
}

void ClientManager::checkDisconnectedClients(bool running) {
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
            networkManager->sendMessage(client->socket, client->playerNumber, msgType, message);
        }
    }
}

void ClientManager::sendToPlayer(int playerNumber, const std::string& msgType, const std::string& message) {
    std::lock_guard<std::mutex> lock(clientsMutex);

    for (auto* client : clients) {
        if (client && client->playerNumber == playerNumber && client->connected) {
            networkManager->sendMessage(client->socket, client->playerNumber, msgType, message);
            return;
        }
    }

    std::cerr << "⚠ Hráč #" << playerNumber << " nebyl nalezen" << std::endl;
}

nlohmann::json ClientManager::findPacketID(int clientNumber, int packetID) {
    std::vector<nlohmann::json> packets = networkManager->getPackets();  // Reference pro efektivitu

    if (packetID == -1) {
        for (int i = packets.size() - 1; i >= 0; i--) {
            int packetClient = static_cast<int>(packets[i]["clientID"]);
            if (packetClient == clientNumber) {
                nlohmann::json data;
                data["packetID"] = packetClient;
                return data;
            }
        }
    } else {
        for (int i = packets.size() - 1; i >= 0; i--) {
            int packetClient = static_cast<int>(packets[i]["clientID"]);
            int actualPacketID = static_cast<int>(packets[i]["packetID"]);
            if (packetClient == clientNumber && packetID == actualPacketID) {
                nlohmann::json data;
                data["packetID"] = packetClient;
                return data;
            }
        }
    }

    return {};
}

void ClientManager::sendLossPackets(ClientInfo* client, int lastReceivedPacketID) {
    std::cout << "\n🔄 Zjišťuji ztracené packety pro klienta #" << client->playerNumber << std::endl;
    std::cout << "   Poslední přijatý packet: " << lastReceivedPacketID << std::endl;

    // Najdeme nejnovější packet ID pro tohoto klienta
    int latestPacketID = networkManager->findLatestPacketID(client->playerNumber);

    if (latestPacketID == -1) {
        std::cout << "   ℹ️ Žádné packety k odeslání" << std::endl;
        return;
    }

    std::cout << "   Nejnovější packet: " << latestPacketID << std::endl;

    // Pokud je klient aktuální, nic neposíláme
    if (lastReceivedPacketID >= latestPacketID) {
        std::cout << "   ✅ Klient je aktuální" << std::endl;
        return;
    }

    // Sebereme všechny chybějící packety
    std::vector<nlohmann::json> missingPackets;

    // Procházíme od (lastReceived + 1) do latest (včetně)
    for (int id = lastReceivedPacketID + 1; id <= latestPacketID; id++) {
        // Ošetření wraparound (pokud ID překročilo 255)
        int actualID = id % NetworkManager::MAXIMUM_PACKET_SIZE;

        nlohmann::json packet = networkManager->findPacketByID(client->playerNumber, actualID);

        if (!packet.empty()) {
            missingPackets.push_back(packet);
            std::cout << "   📦 Našel packet ID:" << actualID << " (type: " << packet["type"] << ")" << std::endl;
        } else {
            std::cerr << "   ⚠️ Packet ID:" << actualID << " nenalezen nebo přepsán" << std::endl;
        }
    }

    if (latestPacketID < lastReceivedPacketID) {
        std::cout << "   🔄 Detekován wraparound" << std::endl;

        // Nejdřív od (lastReceived + 1) do 254
        for (int id = lastReceivedPacketID + 1; id < NetworkManager::MAXIMUM_PACKET_SIZE; id++) {
            nlohmann::json packet = networkManager->findPacketByID(client->playerNumber, id);
            if (!packet.empty()) {
                missingPackets.push_back(packet);
                std::cout << "   📦 Našel packet ID:" << id << " (wraparound)" << std::endl;
            }
        }

        // Pak od 0 do latest
        for (int id = 0; id <= latestPacketID; id++) {
            nlohmann::json packet = networkManager->findPacketByID(client->playerNumber, id);
            if (!packet.empty()) {
                missingPackets.push_back(packet);
                std::cout << "   📦 Našel packet ID:" << id << " (wraparound)" << std::endl;
            }
        }
    }

    std::cout << "   📊 Celkem nalezeno " << missingPackets.size() << " chybějících paketů" << std::endl;

    // Posíláme packety ve správném pořadí (od nejstaršího po nejnovější)
    for (const auto& packet : missingPackets) {
        std::string msgType = packet["type"];
        std::string msgData = packet["data"].dump();

        std::cout << "   📤 Posílám packet ID:" << packet["id"]
                  << " (type: " << msgType << ")" << std::endl;

        networkManager->sendMessage(client->socket, client->playerNumber, msgType, msgData);

        // Malá pauza mezi packety pro zabránění zahlcení
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    std::cout << "   ✅ Znovuposlání dokončeno\n" << std::endl;
}
