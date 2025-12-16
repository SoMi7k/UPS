#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <sstream>
#include <iomanip>
#include <cstring>

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

std::vector<u_int8_t> NetworkManager::findPacketByID(int clientNumber, int packetID) {
    // Kontrola rozsahu
    if (packetID < 0 || packetID >= MAXIMUM_PACKET_SIZE) {
        return {};
    }

    // Získáme packet na dané pozici
    const auto& packet = packets[packetID];
    Protocol::Message msg = Protocol::deserialize(packet);

    // Kontrola zda packet existuje a patří správnému klientovi
    if (packet.empty()) {
        return {};
    }

    int packetClientID = msg.clientID;
    if (packetClientID == clientNumber) {
        return packet;
    }

    return {};
}

int NetworkManager::findLatestPacketID(int clientNumber) {
    int latestID = -1;

    // Procházíme odzadu od aktuálního packetID
    int currentID = (packetID - 1 + MAXIMUM_PACKET_SIZE) % MAXIMUM_PACKET_SIZE;

    for (int i = 0; i < MAXIMUM_PACKET_SIZE; i++) {
        const auto& packet = packets[currentID];

        Protocol::Message msg = Protocol::deserialize(packet);
        if (!packet.empty()) {
            int packetClientID = msg.clientID;
            if (packetClientID == clientNumber) {
                latestID = msg.packetID;
                break;
            }
        }

        currentID = (currentID - 1 + MAXIMUM_PACKET_SIZE) % MAXIMUM_PACKET_SIZE;
    }

    return latestID;
}

bool NetworkManager::sendMessage(int socket, int clientNumber, Protocol::MessageType msgType, std::vector<std::string> msg) {
    // Převedeme na Protocol::Message
    Protocol::Message message = Protocol::createMessage(
        static_cast<uint8_t>(packetID),
        static_cast<uint8_t>(clientNumber),
        msgType,
        msg
    );

    // Serializace do binárního bufferu
    std::vector<uint8_t> buffer = Protocol::serialize(message);

    // Uložíme packet před odesláním
    if (clientNumber != -1) {
        packets[packetID] = buffer;
    }

    std::cout << "📤 Posílám packet ID:" << packetID << " klientovi #" << clientNumber
        << " (type: " << static_cast<int>(message.type) << ")" << std::endl;

    // Inkrementace ID s wraparoundem
    packetID = (packetID + 1) % MAXIMUM_PACKET_SIZE;

    // Odeslání binárních dat
    ssize_t sent = send(socket, buffer.data(), buffer.size(), MSG_NOSIGNAL);

    if (sent <= 0) {
        std::cerr << "❌ Send selhal, socket mrtvý\n";
        return false;
    }

    if (sent != static_cast<ssize_t>(buffer.size())) {
        std::cerr << "❌ Neúplné odeslání packetu\n";
        return false;
    }

    return true;
}

std::vector<uint8_t> NetworkManager::receiveMessage(int socket) {
    std::vector<uint8_t> buffer;

    // 1️⃣ Načti size (2B)
    uint8_t sizeBytes[2];
    if (!recvExact(socket, sizeBytes, 2)) {
        std::cout << "🔌 receiveMessage: Selhalo čtení velikosti zprávy" << std::endl;
        return {};  // Prázdný buffer = odpojení
    }

    uint16_t msgSize = sizeBytes[0] | (sizeBytes[1] << 8);

    if (msgSize < Protocol::HEADER_SIZE || msgSize > Protocol::MAX_MESSAGE_SIZE) {
        std::cerr << "❌ [NET] Neplatná velikost: " << msgSize << std::endl;
        return {};
    }

    buffer.resize(msgSize);
    buffer[0] = sizeBytes[0];
    buffer[1] = sizeBytes[1];

    // 2️⃣ Načti zbytek
    if (!recvExact(socket, buffer.data() + 2, msgSize - 2)) {
        std::cout << "🔌 receiveMessage: Selhalo čtení těla zprávy" << std::endl;
        return {};  // Prázdný buffer = odpojení
    }

    std::cout << "✅ Přijat packet: " << debug_print_bytes(buffer) << std::endl;

    return buffer;
}

bool NetworkManager::recvExact(int socket, uint8_t* buffer, size_t len) {
    size_t total = 0;

    while (total < len) {
        ssize_t r = recv(socket, buffer + total, len - total, 0);

        // 🔴 DETEKCE ODPOJENÍ
        if (r == 0) {
            // Socket byl zavřen klientem (graceful shutdown)
            std::cout << "🔌 Socket " << socket << " byl zavřen klientem (recv vrátil 0)" << std::endl;
            return false;
        }

        if (r < 0) {
            // Chyba při čtení
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                std::cout << "⏱️ Socket " << socket << " timeout" << std::endl;
            } else if (errno == ECONNRESET) {
                std::cout << "🔌 Socket " << socket << " - connection reset by peer" << std::endl;
            } else if (errno == EPIPE) {
                std::cout << "🔌 Socket " << socket << " - broken pipe" << std::endl;
            } else {
                std::cerr << "❌ Socket " << socket << " chyba: "
                          << strerror(errno) << " (errno: " << errno << ")" << std::endl;
            }
            return false;
        }

        total += r;
    }

    return true;
}

// Pomocná funkce pro konverzi std::vector<uint8_t> na debugovací string (např. "b'\x0b\x00...'")
std::string NetworkManager::debug_print_bytes(const std::vector<uint8_t>& buffer) {
    std::stringstream ss;
    ss << "b'";

    // Procházíme všechny byty ve vektoru
    for (size_t i = 0; i < buffer.size(); ++i) {
        uint8_t byte = buffer[i];

        // Zkusíme tisknout čitelné ASCII znaky (pro lepší přehlednost)
        if (byte >= 32 && byte <= 126 && byte != 39 && byte != 92) { // 39=' / 92=\
            ss << static_cast<char>(byte);
        } else {
            // Tiskneme hexadecimálně \xXX
            ss << "\\x" << std::hex << std::setw(2) << std::setfill('0') << (int)byte;
        }
    }
    ss << "'";
    return ss.str();
}