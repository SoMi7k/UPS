#include "NetworkManager.hpp"
#include <iostream>
#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ifaddrs.h>
#include <net/if.h>

#define QUEUE_LENGTH 10

NetworkManager::NetworkManager(int port)
    : serverSocket(-1), port(port) {
    std::cout << "🔧 NetworkManager vytvořen na portu " << port << std::endl;
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
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        std::cerr << "Chyba při vytváření socketu" << std::endl;
        return false;
    }
    std::cout << "Socket vytvořen (fd: " << serverSocket << ")" << std::endl;

    int opt = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "Chyba při nastavení SO_REUSEADDR" << std::endl;
        close(serverSocket);
        return false;
    }

    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    std::cout << "\n📡 Server bude naslouchat na:" << std::endl;
    auto addresses = getLocalIPAddresses();
    for (const auto& addr : addresses) {
        std::cout << "   -> " << addr << ":" << port << std::endl;
    }
    if (addresses.empty()) {
        std::cout << "   -> 127.0.0.1:" << port << " (pouze localhost)" << std::endl;
    }
    std::cout << std::endl;

    if (bind(serverSocket, reinterpret_cast<sockaddr*>(&serverAddress),
             sizeof(serverAddress)) < 0) {
        std::cerr << "Chyba při bindování socketu" << std::endl;
        close(serverSocket);
        return false;
    }

    if (listen(serverSocket, QUEUE_LENGTH) < 0) {
        std::cerr << "Chyba při naslouchání" << std::endl;
        close(serverSocket);
        return false;
    }

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

bool NetworkManager::sendMessage(int socket, const std::string& msgType, const std::string& message) {
    try {
        nlohmann::json msg;
        msg["type"] = msgType;

        try {
            msg["data"] = nlohmann::json::parse(message);
        } catch (...) {
            msg["data"] = message;
        }

        msg["timestamp"] = std::time(nullptr);

        std::string serialized = msg.dump() + "\n";
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