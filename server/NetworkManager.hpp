#ifndef NETWORK_MANAGER_HPP
#define NETWORK_MANAGER_HPP

#include <string>
#include <vector>

#include "Protocol.hpp"

// Třída zajišťující síťovou komunikaci serveru
class NetworkManager {
public:

    // Konstruktor – uloží IP adresu a port serveru
    NetworkManager(const std::string& ip, int port);

    // Destruktor – uvolnění prostředků
    ~NetworkManager();

    // Maximální velikost síťového paketu
    static constexpr int MAXIMUM_PACKET_SIZE = 255;

    enum class ValidationResult {
        VALID = 0,
        INVALID_CLIENT_ID = 1,
        INVALID_PACKET_ID = 2,
        INVALID_MESSAGE_TYPE = 3,
        INVALID_PACKET_SEQUENCE = 4,
        MALFORMED_DATA = 5,
        INVALID_FIELD_COUNT = 6,
        INVALID_CHARACTERS = 7,
        MESSAGE_TOO_LARGE = 8
    };

    bool isValidMessageString(const std::string& data); // Kontrola stringu před deserializací
    int Validation(const Protocol::Message & msg, int clientNumber, int requiredPlayers);
    // Validace zprávy s důvodem selhání
    ValidationResult validateMessage(const Protocol::Message &msg,
                         int clientNumber,
                         int requiredPlayers);

    // ===== Socket operace =====
    bool initializeSocket(); // Inicializace serverového socketu
    void closeSocket(int socket); // Uzavře konkrétní socket
    void closeServerSocket(); // Uzavře serverový socket
    bool enableKeepAlive(int socket);

    // ===== Práce se zprávami =====
    bool sendMessage(int socket, int clientNumber, Protocol::MessageType msgType,
                    std::vector<std::string> msg); // Odešle zprávu klientovi podle protokolu
    std::string receiveMessage(int socket); // Přijme zprávu od klienta

    // ===== Práce s pakety =====
    std::string findPacketByID(int clientNumber, int packetID); // Najde paket podle ID klienta a ID paketu
    int findLatestPacketID(int clientNumber); // Vrátí ID posledního paketu pro daného klienta

    // ===== Gettery =====
    int getServerSocket() const { return serverSocket; }
    int getPort() const { return port; }
    const std::vector<std::string>& getPackets() const { return packets; }
    int getCurrentPacketID() const { return packetID; }

private:
    std::string bindIP;                            // IP adresa serveru
    int serverSocket;                              // Serverový socket
    int port;                                      // Port serveru
    int packetID;                                  // Aktuální ID paketu
    std::vector<std::string> packets;    // Uložené pakety


    static std::vector<std::string> getLocalIPAddresses(); // Získá seznam lokálních IP adres
    static bool containsSuspiciousPatterns(const std::string& str); // Pomocné validační funkce
};

#endif // NETWORK_MANAGER_HPP
