#include <iostream>
#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ifaddrs.h>
#include <net/if.h>

#include "NetworkManager.hpp"

#define QUEUE_LENGTH 10

// 🆕 Konstruktor s IP adresou
NetworkManager::NetworkManager(const std::string& ip, int port)
    : bindIP(ip), serverSocket(-1), port(port), packetID(0) {

    packets.resize(MAXIMUM_PACKET_SIZE, {});
    std::cout << "🔧 NetworkManager inicializován" << std::endl;
    std::cout << "   - Bind IP: " << bindIP << std::endl;
    std::cout << "   - Port: " << port << std::endl;
}

NetworkManager::~NetworkManager() {
    closeServerSocket();
}

std::vector<std::string> NetworkManager::getLocalIPAddresses() {
    std::vector<std::string> addresses;
    struct ifaddrs* ifAddrStruct = nullptr;
    struct ifaddrs* ifa = nullptr;

    if (getifaddrs(&ifAddrStruct) == -1) {
        std::cerr << "Chyba při získávání IP adres" << std::endl;
        return addresses;
    }

    for (ifa = ifAddrStruct; ifa != nullptr; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr) {
            continue;
        }

        if (ifa->ifa_addr->sa_family == AF_INET) {
            void* tmpAddrPtr = &((struct sockaddr_in*)ifa->ifa_addr)->sin_addr;
            char addressBuffer[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);

            std::string addr = addressBuffer;
            if (addr != "127.0.0.1") {
                addresses.push_back(addr);
                std::cout << "🌐 Rozhraní: " << ifa->ifa_name
                          << " -> IP: " << addr << std::endl;
            }
        }
    }

    if (ifAddrStruct != nullptr) {
        freeifaddrs(ifAddrStruct);
    }

    return addresses;
}

bool NetworkManager::initializeSocket() {
    std::cout << "🔌 Inicializuji socket..." << std::endl;

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        std::cerr << "❌ Nepodařilo se vytvořit socket" << std::endl;
        return false;
    }

    // Nastavení SO_REUSEADDR
    int opt = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "⚠️  Varování: Nepodařilo se nastavit SO_REUSEADDR" << std::endl;
    }

    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);

    // 🆕 Použití specifikované IP místo INADDR_ANY
    if (inet_pton(AF_INET, bindIP.c_str(), &serverAddress.sin_addr) <= 0) {
        std::cerr << "❌ Neplatná IP adresa: " << bindIP << std::endl;
        close(serverSocket);
        return false;
    }

    std::cout << "  -> Binding na " << bindIP << ":" << port << std::endl;

    if (bind(serverSocket, (sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        std::cerr << "❌ Bind selhal (možná je port " << port << " již používán)" << std::endl;
        close(serverSocket);
        return false;
    }

    if (listen(serverSocket, 10) < 0) {
        std::cerr << "❌ Listen selhal" << std::endl;
        close(serverSocket);
        return false;
    }

    std::cout << "✅ Socket úspěšně inicializován na " << bindIP << ":" << port << std::endl;
    return true;
}

int NetworkManager::acceptConnection() {
    sockaddr_in clientAddress{};
    socklen_t clientLen = sizeof(clientAddress);

    int clientSocket = accept(serverSocket,
                              reinterpret_cast<sockaddr*>(&clientAddress),
                              &clientLen);

    if (clientSocket < 0) {
        return -1;
    }

    char clientIP[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &clientAddress.sin_addr, clientIP, INET_ADDRSTRLEN);

    std::cout << "\n✓ Nový klient se připojil!" << std::endl;
    std::cout << "  - Socket: " << clientSocket << std::endl;
    std::cout << "  - IP: " << clientIP << std::endl;
    std::cout << "  - Port: " << ntohs(clientAddress.sin_port) << std::endl;

    return clientSocket;
}

void NetworkManager::closeSocket(int socket) {
    if (socket >= 0) {
        shutdown(socket, SHUT_RDWR);
        close(socket);
    }
}

void NetworkManager::closeServerSocket() {
    if (serverSocket >= 0) {
        std::cout << "🔌 Zavírám hlavní socket..." << std::endl;
        shutdown(serverSocket, SHUT_RDWR);
        close(serverSocket);
        serverSocket = -1;
    }
}

nlohmann::json NetworkManager::findPacketByID(int clientNumber, int packetID) {
    // Kontrola rozsahu
    if (packetID < 0 || packetID >= MAXIMUM_PACKET_SIZE) {
        return nlohmann::json();
    }

    // Získáme packet na dané pozici
    const auto& packet = packets[packetID];

    // Kontrola zda packet existuje a patří správnému klientovi
    if (packet.empty() || !packet.contains("clientID")) {
        return nlohmann::json();
    }

    int packetClientID = static_cast<int>(packet["clientID"]);
    if (packetClientID == clientNumber) {
        return packet;
    }

    return nlohmann::json();
}

int NetworkManager::findLatestPacketID(int clientNumber) {
    int latestID = -1;

    // Procházíme odzadu od aktuálního packetID
    int currentID = (packetID - 1 + MAXIMUM_PACKET_SIZE) % MAXIMUM_PACKET_SIZE;

    for (int i = 0; i < MAXIMUM_PACKET_SIZE; i++) {
        const auto& packet = packets[currentID];

        if (!packet.empty() && packet.contains("clientID")) {
            int packetClientID = static_cast<int>(packet["clientID"]);
            if (packetClientID == clientNumber) {
                latestID = static_cast<int>(packet["id"]);
                break;
            }
        }

        currentID = (currentID - 1 + MAXIMUM_PACKET_SIZE) % MAXIMUM_PACKET_SIZE;
    }

    return latestID;
}

bool NetworkManager::sendMessage(int socket, int clientNumber, const std::string& msgType, const std::string& message) {
    try {
        nlohmann::json msg;
        msg["id"] = packetID;
        msg["type"] = msgType;
        msg["clientID"] = clientNumber;

        try {
            msg["data"] = nlohmann::json::parse(message);
        } catch (...) {
            msg["data"] = message;
        }

        msg["timestamp"] = std::time(nullptr);

        // Uložíme packet před odesláním
        if (clientNumber != -1) {
           packets[packetID] = msg;
        }

        std::string serialized = msg.dump() + "\n";

        std::cout << "📤 Posílám packet ID:" << packetID
                  << " klientovi #" << clientNumber
                  << " (type: " << msgType << ")" << std::endl;

        // Inkrementace s wraparoundem
        packetID = (packetID + 1) % MAXIMUM_PACKET_SIZE;

        ssize_t bytesSent = send(socket, serialized.c_str(), serialized.size(), 0);
        return bytesSent == (ssize_t)serialized.size();
    }
    catch (const std::exception& e) {
        std::cerr << "❌ Chyba odeslání: " << e.what() << std::endl;
        return false;
    }
}

std::string NetworkManager::receiveMessage(int socket) {
    std::string buffer;
    char chunk[256];
    ssize_t bytesReceived;

    while (true) {
        bytesReceived = recv(socket, chunk, sizeof(chunk), 0);

        if (bytesReceived <= 0) {
            return "";
        }

        buffer.append(chunk, bytesReceived);

        size_t pos = buffer.find('\n');
        if (pos != std::string::npos) {
            std::string message = buffer.substr(0, pos);
            return message;
        }
    }
}

nlohmann::json NetworkManager::deserialize(const std::string& msg) {
    if (msg.empty()) {
        return nlohmann::json{};
    }

    try {
        nlohmann::json parsed = nlohmann::json::parse(msg);
        std::cout << "📥 Typ zprávy: " << parsed["type"] << std::endl;

        if (parsed.contains("data")) {
            std::cout << "📥 Data: " << parsed["data"] << std::endl;
        }
        return parsed;

    } catch (const std::exception& e) {
        std::cerr << "❌ Chyba při parsování JSON: " << e.what() << std::endl;
        return nlohmann::json{};
    }
}

std::string NetworkManager::serialize(const std::string& msgType, const nlohmann::json& data) {
    nlohmann::json msg;
    msg["type"] = msgType;
    msg["data"] = data;
    msg["timestamp"] = std::time(nullptr);
    return msg.dump();
}