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
    : bindIP(ip), serverSocket(-1), port(port), packetID(1) {

    packets.resize(MAXIMUM_PACKET_SIZE, {});
    std::cout << "🔧 NetworkManager inicializován" << std::endl;
    std::cout << "   - Bind IP: " << bindIP << std::endl;
    std::cout << "   - Port: " << port << std::endl;
}

NetworkManager::~NetworkManager() {
    closeServerSocket();
}

#include <algorithm>
#include <cctype>
#include <regex>

bool NetworkManager::isValidMessageString(const std::string& data) {
    // === 1. Kontrola prázdné zprávy ===
    if (data.empty()) {
        std::cerr << "❌ [VALIDATION] Prázdná zpráva" << std::endl;
        return false;
    }

    // === 2. Kontrola délky ===
    if (data.length() > Protocol::MAX_MESSAGE_SIZE) {
        std::cerr << "❌ [VALIDATION] Zpráva příliš dlouhá: "
                  << data.length() << " > " << Protocol::MAX_MESSAGE_SIZE << std::endl;
        return false;
    }

    // === 3. Kontrola terminátoru ===
    if (data.back() != Protocol::TERMINATOR) {
        std::cerr << "❌ [VALIDATION] Chybí terminátor \\n" << std::endl;
        return false;
    }

    // === 4. Počet delimiterů (minimálně 3: SIZE|PACKET|CLIENT|TYPE) ===
    int delimiterCount = std::count(data.begin(), data.end(), Protocol::DELIMITER);
    if (delimiterCount < 2) {
        std::cerr << "❌ [VALIDATION] Nedostatek delimiterů: "
                  << delimiterCount << " < 2" << std::endl;
        return false;
    }

    // === 5. Kontrola neplatných znaků (null bytes, kontrolní znaky kromě \n) ===
    for (size_t i = 0; i < data.length(); i++) {
        unsigned char c = data[i];

        // Null byte
        if (c == 0) {
            std::cerr << "❌ [VALIDATION] Null byte na pozici " << i << std::endl;
            return false;
        }

        // Kontrolní znaky (kromě \n a \r)
        if (c < 32 && c != '\n' && c != '\r') {
            std::cerr << "❌ [VALIDATION] Neplatný kontrolní znak: "
                      << static_cast<int>(c) << " na pozici " << i << std::endl;
            return false;
        }
    }

    // === 6. Kontrola parsovatelnosti první části (SIZE) ===
    size_t firstDelim = data.find(Protocol::DELIMITER);
    if (firstDelim == std::string::npos) {
        return false;
    }

    std::string sizeStr = data.substr(0, firstDelim);

    // SIZE musí být číslo
    if (sizeStr.empty() || !std::all_of(sizeStr.begin(), sizeStr.end(), ::isdigit)) {
        std::cerr << "❌ [VALIDATION] SIZE není číslo: '" << sizeStr << "'" << std::endl;
        return false;
    }

    // === 7. Kontrola podezřelých vzorů (opakující se znaky = spam) ===
    if (containsSuspiciousPatterns(data)) {
        std::cerr << "❌ [VALIDATION] Detekován podezřelý vzor (spam)" << std::endl;
        return false;
    }

    return true;
}

bool NetworkManager::containsSuspiciousPatterns(const std::string& str) {
    // Kontrola opakujících se znaků (100+ stejných znaků za sebou = spam)
    int consecutiveCount = 1;
    char lastChar = 0;

    for (char c : str) {
        if (c == lastChar) {
            consecutiveCount++;
            if (consecutiveCount > 100) {
                std::cout << "⚠️ [VALIDATION] Detekováno " << consecutiveCount
                          << " opakujících se znaků" << std::endl;
                return true;  // Podezřelé
            }
        } else {
            consecutiveCount = 1;
            lastChar = c;
        }
    }

    return false;
}

NetworkManager::ValidationResult NetworkManager::validateMessage(
    const Protocol::Message& msg,
    int clientNumber,
    int requiredPlayers) {

    std::cout << "🔍 [VALIDATION] Validuji zprávu od klienta #" << clientNumber << std::endl;
    std::cout << "   - PacketID: " << static_cast<int>(msg.packetID) << std::endl;
    std::cout << "   - ClientID: " << static_cast<int>(msg.clientID) << std::endl;
    std::cout << "   - Type: " << static_cast<int>(msg.type) << std::endl;
    std::cout << "   - Fields: " << msg.fields.size() << std::endl;

    // === 1. KONTROLA CLIENT ID ===
    // ClientID musí odpovídat očekávanému číslu klienta
    if (msg.clientID != clientNumber) {
        std::cerr << "❌ [VALIDATION] ClientID nesouhlasí: "
                  << static_cast<int>(msg.clientID) << " != " << clientNumber << std::endl;
        return ValidationResult::INVALID_CLIENT_ID;
    }

    // ClientID musí být v platném rozsahu
    if (msg.clientID >= requiredPlayers) {
        std::cerr << "❌ [VALIDATION] ClientID mimo rozsah: "
                  << static_cast<int>(msg.clientID) << " >= " << requiredPlayers << std::endl;
        return ValidationResult::INVALID_CLIENT_ID;
    }

    // === 2. KONTROLA MESSAGE TYPE ===
    // Type musí být validní (0-19)
    int typeValue = static_cast<int>(msg.type);
    if (typeValue < 0 || typeValue > 19) {
        std::cerr << "❌ [VALIDATION] Neplatný typ zprávy: " << typeValue << std::endl;
        return ValidationResult::INVALID_MESSAGE_TYPE;
    }

    // === 3. KONTROLA PACKET ID SEKVENCE ===
    // PacketID by měl postupovat logicky (s tolerancí pro wraparound)
    if (clientNumber >= 0) {
        int lastPacketID = findLatestPacketID(clientNumber);

        if (lastPacketID != -1) {
            // Spočítej očekávané ID (s wraparoundem)
            int expectedID = (lastPacketID + 1) % MAXIMUM_PACKET_SIZE;

            // Toleruj malé rozdíly (kvůli retransmisi nebo ztrátě packetu)
            int diff = std::abs(msg.packetID - expectedID);

            // Pokud je rozdíl větší než 10, je to podezřelé
            if (diff > 10 && diff < (MAXIMUM_PACKET_SIZE - 10)) {
                std::cerr << "⚠️ [VALIDATION] Podezřelá sekvence packetID: "
                          << static_cast<int>(msg.packetID) << " (očekáváno ~"
                          << expectedID << ")" << std::endl;
                // Toto není fatální chyba, ale logujeme ji
            }
        }
    }

    // === 5. KONTROLA OBSAHU FIELDS ===
    for (size_t i = 0; i < msg.fields.size(); i++) {
        const std::string& field = msg.fields[i];

        // Field nesmí být příliš dlouhý
        if (field.length() > 1000) {
            std::cerr << "❌ [VALIDATION] Field " << i << " je příliš dlouhý: "
                      << field.length() << " znaků" << std::endl;
            return ValidationResult::MALFORMED_DATA;
        }

        // Field nesmí obsahovat null bytes
        if (field.find('\0') != std::string::npos) {
            std::cerr << "❌ [VALIDATION] Field " << i << " obsahuje null byte" << std::endl;
            return ValidationResult::INVALID_CHARACTERS;
        }

        // Field nesmí obsahovat delimiter nebo terminator
        if (field.find(Protocol::DELIMITER) != std::string::npos ||
            field.find(Protocol::TERMINATOR) != std::string::npos) {
            std::cerr << "❌ [VALIDATION] Field " << i
                      << " obsahuje zakázané znaky (| nebo \\n)" << std::endl;
            return ValidationResult::INVALID_CHARACTERS;
        }
    }

    // === 6. KONTROLA CELKOVÉ VELIKOSTI ===
    if (msg.size > Protocol::MAX_MESSAGE_SIZE) {
        std::cerr << "❌ [VALIDATION] Zpráva příliš velká: "
                  << msg.size << " > " << Protocol::MAX_MESSAGE_SIZE << std::endl;
        return ValidationResult::MESSAGE_TOO_LARGE;
    }

    std::cout << "✅ [VALIDATION] Zpráva validní" << std::endl;
    return ValidationResult::VALID;
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

std::string NetworkManager::findPacketByID(int clientNumber, int packetID) {
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

bool NetworkManager::sendMessage(int socket, int clientNumber,
                                Protocol::MessageType msgType,
                                std::vector<std::string> msg) {
    // Vytvoříme zprávu
    Protocol::Message message = Protocol::createMessage(
        static_cast<uint8_t>(packetID),
        static_cast<uint8_t>(clientNumber),
        msgType,
        msg
    );

    // Serializujeme do textového formátu
    std::string textData = Protocol::serialize(message);

    // Uložíme do historie (pro reconnect)
    if (clientNumber != -1) {
        packets[packetID] = textData;  // 🆕 Uložíme jako string
    }

    std::cout << "📤 Posílám packet ID:" << packetID
              << " klientovi #" << clientNumber
              << " (type: " << static_cast<int>(message.type) << ")" << std::endl;
    std::cout << "   Data: " << textData << std::endl;

    // Inkrementace ID
    packetID = (packetID + 1) % MAXIMUM_PACKET_SIZE;

    // Odeslání textových dat
    ssize_t sent = send(socket, textData.c_str(), textData.length(), MSG_NOSIGNAL);

    if (sent <= 0) {
        std::cerr << "❌ Send selhal, socket mrtvý" << std::endl;
        return false;
    }

    if (sent != static_cast<ssize_t>(textData.length())) {
        std::cerr << "❌ Neúplné odeslání packetu" << std::endl;
        return false;
    }

    return true;
}

static bool readUntilNewline(int socket, std::string& output) {
    output.clear();
    char buffer[1];

    while (true) {
        ssize_t r = recv(socket, buffer, 1, 0);

        // 🔴 Detekce odpojení
        if (r == 0) {
            std::cout << "🔌 Socket " << socket << " byl zavřen" << std::endl;
            return false;
        }

        if (r < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                std::cout << "⏱️ Socket " << socket << " timeout" << std::endl;
            } else {
                std::cerr << "❌ Socket " << socket << " chyba: "
                          << strerror(errno) << std::endl;
            }
            return false;
        }

        // Přidáme znak
        output += buffer[0];

        // Pokud jsme našli \n, hotovo
        if (buffer[0] == Protocol::TERMINATOR) {
            return true;
        }

        // Ochrana proti příliš dlouhým zprávám
        if (output.length() > Protocol::MAX_MESSAGE_SIZE) {
            std::cerr << "❌ Zpráva příliš dlouhá" << std::endl;
            return false;
        }
    }
}

std::string NetworkManager::receiveMessage(int socket) {
    std::string data;

    if (!readUntilNewline(socket, data)) {
        std::cout << "🔌 receiveMessage: Selhalo čtení zprávy" << std::endl;
        return "";
    }

    std::cout << "✅ Přijata zpráva: " << data << std::endl;
    return data;
}

int NetworkManager::checkMessage(Protocol::Message msg, int clientNumber, int required_players) {
    if (msg.clientID > required_players - 1) {
        std::cout << msg.clientID << " : " << required_players << std::endl;
        return 0;
    }

    if (msg.packetID) {
        int lastpacketID = findLatestPacketID(clientNumber);

        if (lastpacketID == -1) {
            std::cout << msg.packetID << " : " << lastpacketID << std::endl;
            return 0;
        }
    }

    if (static_cast<int>(msg.type) > 19) {
        std::cout << static_cast<int>(msg.type) << " : " << 19 << std::endl;
        return 0;
    }

    return 1;
}