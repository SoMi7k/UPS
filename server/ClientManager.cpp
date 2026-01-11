#include "ClientManager.hpp"
#include "NetworkManager.hpp"
#include <iostream>
#include <algorithm>
#include <unistd.h>
#include <sys/socket.h>

ClientManager::ClientManager(int requiredPlayers, NetworkManager* networkManager)
    : networkManager(networkManager), requiredPlayers(requiredPlayers), connectedPlayers(0) {
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

// ============================================================
// ADD & REMOVE CLIENT
// ============================================================
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
        false,
        std::chrono::steady_clock::now(),
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

// ============================================================
// DISCONNECTION
// ============================================================
void ClientManager::disconnectAll() {
    std::vector<ClientInfo*> clientsCopy;
    {
        std::lock_guard<std::mutex> lock(clientsMutex);
        clientsCopy = clients;
        std::cout << "🔌 Odpojuji " << clientsCopy.size() << " klientů..." << std::endl;
    }

    for (auto* client : clientsCopy) {
        if (client && client->connected) {
            networkManager->sendMessage(client->socket, client->playerNumber, Protocol::MessageType::DISCONNECT, {"Server se vypíná"});
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

    // Notifikace ostatních - teď je bezpečná
    std::vector<std::string> statusData;
    statusData.emplace_back("1"); // code
    statusData.emplace_back(client->nickname);
    statusData.emplace_back(std::to_string(connectedPlayers));

    if (client->approved) {
        for (auto c : clients) {
            if (c->playerNumber != client->playerNumber) {
                sendToPlayer(c->playerNumber, Protocol::MessageType::STATUS, statusData);
            }
        }
        authorizeCount--;
    }

    std::cout << "✅ Hráč #" << client->playerNumber << " odpojen" << std::endl;
    std::cout << std::string(50, '-') << std::endl;
}

// ============================================================
// FINDERS
// ============================================================
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

// ============================================================
// VÝPADEK & RECONNECTION
// ============================================================
bool ClientManager::reconnectClient(ClientInfo* oldClient, int newSocket) {
    if (!oldClient) return false;

    std::cout << "🔄 Reconnecting hráče #" << oldClient->playerNumber << std::endl;

    {
        std::lock_guard<std::mutex> lock(clientsMutex);

        // Najdi klienta s tímto socketem (má playerNumber = -1)
        auto it = std::find_if(clients.begin(), clients.end(),
            [newSocket](ClientInfo* c) {
                return c && c->socket == newSocket && c->playerNumber == -1;
            });

        if (it != clients.end()) {
            std::cout << "🗑️ Odstraňuji dočasného klienta s socketem " << newSocket << std::endl;
            delete *it;
            clients.erase(it);
            connectedPlayers--;
        }
    }

    // Zavři starý socket
    if (oldClient->socket >= 0 && oldClient->socket != newSocket) {
        close(oldClient->socket);
    }

    // Nastav nový socket
    oldClient->socket = newSocket;
    oldClient->connected = true;
    oldClient->isDisconnected = false;
    oldClient->lastSeen = std::chrono::steady_clock::now();

    // Status broadcast
    std::vector<std::string> statusData;
    statusData.emplace_back("3");
    statusData.emplace_back(oldClient->nickname);

    for (auto c : clients) {
        if (c->playerNumber != oldClient->playerNumber) {
            sendToPlayer(c->playerNumber, Protocol::MessageType::STATUS, statusData);
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

    std::vector<std::string> statusData;
    statusData.emplace_back("2"); // code
    statusData.emplace_back(client->nickname);
    statusData.emplace_back(std::to_string(RECONNECT_TIMEOUT_SECONDS));

    for (auto& c : clients) {
        if (c->playerNumber != client->playerNumber) {
            sendToPlayer(c->playerNumber, Protocol::MessageType::STATUS, statusData);
        }
    }

    std::cout << "⏳ Čekám " << RECONNECT_TIMEOUT_SECONDS << "s na reconnect hráče #"
              << client->playerNumber << std::endl;
}

void ClientManager::checkDisconnectedClients(bool running) {
    while (running) {
        std::this_thread::sleep_for(std::chrono::seconds(2));

        std::vector<ClientInfo*> toRemove;
        std::vector<ClientInfo*> toDisconnect;

        {
            std::lock_guard<std::mutex> lock(clientsMutex);
            auto now = std::chrono::steady_clock::now();

            for (auto* client : clients) {
                if (!client) continue;

                auto elapsed = now - client->lastSeen;
                auto seconds = std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();

                // Kontrola autorizačního timeoutu JEN pro nové klienty
                if (client->playerNumber == -1 && !client->approved) {
                    auto timeSinceCreation = now - client->createdAt;
                    auto secondsSinceCreation = std::chrono::duration_cast<std::chrono::seconds>(
                        timeSinceCreation
                    ).count();

                    // Timeout 10s POUZE pro nové klienty (ne pro reconnect)
                    if (secondsSinceCreation >= 10) {
                        std::cout << "⏱️ Klient #" << client->playerNumber
                                  << " se neautorizoval do 10s – odpojuji" << std::endl;
                        toDisconnect.push_back(client);
                        continue;
                    }
                }

                // === Klient je disconnected (čekáme na reconnect) ===
                if (client->isDisconnected) {
                    if (seconds >= RECONNECT_TIMEOUT_SECONDS) {
                        std::cout << "⏱️ Timeout pro odpojeného hráče #" << client->playerNumber
                                  << " (" << seconds << "s) - odstraňuji permanentně" << std::endl;
                        toRemove.push_back(client);
                    } else {
                        std::cout << "⏳ Hráč #" << client->playerNumber
                                  << " odpojený " << seconds << "s / "
                                  << RECONNECT_TIMEOUT_SECONDS << "s" << std::endl;
                    }
                }
            }
        }

        // Označíme jako disconnected
        for (auto* client : toDisconnect) {
            // 🆕 Pokud má playerNumber == -1, odpoj natvrdo (není co reconnectovat)
            if (client->playerNumber == -1) {
                disconnectClient(client);
            } else {
                handleClientDisconnection(client);
            }
        }

        // Permanentně odebereme
        for (auto* client : toRemove) {
            disconnectClient(client);
        }
    }
}

// ============================================================
// GETTERS
// ============================================================
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

// ============================================================
// Funkce k poslání zpráv
// ============================================================
void ClientManager::broadcastMessage(Protocol::MessageType msgType, std::vector<std::string> msg) {
    std::lock_guard<std::mutex> lock(clientsMutex);
    std::cout << "📢 Broadcast: " <<  static_cast<int>(msgType)  << std::endl;

    for (auto* client : clients) {
        if (client && client->connected) {
            networkManager->sendMessage(client->socket, client->playerNumber, msgType, msg);
        }
    }
}

void ClientManager::sendToPlayer(int playerNumber, Protocol::MessageType msgType, std::vector<std::string> msg) {
    std::lock_guard<std::mutex> lock(clientsMutex);

    for (auto* client : clients) {
        if (client && client->playerNumber == playerNumber && client->connected) {
            networkManager->sendMessage(client->socket, client->playerNumber, msgType, msg);
            return;
        }
    }

    std::cerr << "⚠ Hráč #" << playerNumber << " nebyl nalezen" << std::endl;
}

// ============================================================
// Algoritmus pro vrácení paketů
// ============================================================
u_int8_t ClientManager::findPacketID(int clientNumber, int packetID) {
    std::vector<std::string> packets = networkManager->getPackets();  // Reference pro efektivitu

    if (packetID == -1) {
        for (int i = packets.size() - 1; i >= 0; i--) {
            const auto& packet = packets[i];
            Protocol::Message msg = Protocol::deserialize(packet);
            int packetClient = msg.clientID;
            if (packetClient == clientNumber) {
                return msg.packetID;;
            }
        }
    } else {
        for (int i = packets.size() - 1; i >= 0; i--) {
            const auto& packet = packets[i];
            Protocol::Message msg = Protocol::deserialize(packet);
            int packetClient = msg.clientID;
            int actualPacketID = msg.packetID;
            if (packetClient == clientNumber && packetID == actualPacketID) {
                return msg.packetID;
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
    std::vector<std::string> missingPackets;

    // Procházíme od (lastReceived + 1) do latest (včetně)
    for (int id = lastReceivedPacketID + 1; id <= latestPacketID; id++) {
        // Ošetření wraparound (pokud ID překročilo 255)
        int actualID = id % NetworkManager::MAXIMUM_PACKET_SIZE;

        std::string packet = networkManager->findPacketByID(client->playerNumber, actualID);
        Protocol::Message msg = Protocol::deserialize(packet);

        if (!packet.empty()) {
            missingPackets.push_back(packet);
            std::cout << "   📦 Našel packet ID:" << actualID << " (type: " << static_cast<int>(msg.type) << ")" << std::endl;
        } else {
            std::cerr << "   ⚠️ Packet ID:" << actualID << " nenalezen nebo přepsán" << std::endl;
        }
    }

    if (latestPacketID < lastReceivedPacketID) {
        std::cout << "   🔄 Detekován wraparound" << std::endl;

        // Nejdřív od (lastReceived + 1) do 254
        for (int id = lastReceivedPacketID + 1; id < NetworkManager::MAXIMUM_PACKET_SIZE; id++) {
            std::string packet = networkManager->findPacketByID(client->playerNumber, id);
            if (!packet.empty()) {
                missingPackets.push_back(packet);
                std::cout << "   📦 Našel packet ID:" << id << " (wraparound)" << std::endl;
            }
        }

        // Pak od 0 do latest
        for (int id = 0; id <= latestPacketID; id++) {
            std::string packet = networkManager->findPacketByID(client->playerNumber, id);
            if (!packet.empty()) {
                missingPackets.push_back(packet);
                std::cout << "   📦 Našel packet ID:" << id << " (wraparound)" << std::endl;
            }
        }
    }

    std::cout << "   📊 Celkem nalezeno " << missingPackets.size() << " chybějících paketů" << std::endl;

    // Posíláme packety ve správném pořadí (od nejstaršího po nejnovější)
    for (const auto& packet : missingPackets) {
        Protocol::Message msg = Protocol::deserialize(packet);

        std::cout << "   📤 Posílám packet ID:" << msg.packetID
                  << " (type: " << static_cast<int>(msg.type) << ")" << std::endl;

        networkManager->sendMessage(client->socket, client->playerNumber, msg.type, msg.fields);

        // Malá pauza mezi packety pro zabránění zahlcení
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    std::cout << "   ✅ Znovuposlání dokončeno\n" << std::endl;
}
