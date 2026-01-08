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

    // ===== Socket operace =====

    // Inicializace serverového socketu
    bool initializeSocket();

    // Přijme nové klientské připojení
    int acceptConnection();

    // Uzavře konkrétní socket
    void closeSocket(int socket);

    // Uzavře serverový socket
    void closeServerSocket();

    // ===== Práce se zprávami =====

    // Odešle zprávu klientovi podle protokolu
    bool sendMessage(
        int socket,
        int clientNumber,
        Protocol::MessageType msgType,
        std::vector<std::string> msg
    );

    // Přijme zprávu od klienta
    std::vector<uint8_t> receiveMessage(int socket);

    // ===== Práce s pakety =====

    // Najde paket podle ID klienta a ID paketu
    std::vector<u_int8_t> findPacketByID(int clientNumber, int packetID);

    // Vrátí ID posledního paketu pro daného klienta
    int findLatestPacketID(int clientNumber);

    // Zkontroluje hlavičku zprávy
    int checkMessage(Protocol::Message msg, int clientNumber, int required_players);

    // ===== Gettery =====

    // Vrátí serverový socket
    int getServerSocket() const { return serverSocket; }

    // Vrátí port serveru
    int getPort() const { return port; }

    // Vrátí uložené pakety
    const std::vector<std::vector<u_int8_t>>& getPackets() const { return packets; }

    // Vrátí aktuální ID paketu
    int getCurrentPacketID() const { return packetID; }

private:
    std::string bindIP;                            // IP adresa serveru
    int serverSocket;                              // Serverový socket
    int port;                                      // Port serveru
    int packetID;                                  // Aktuální ID paketu
    std::vector<std::vector<u_int8_t>> packets;    // Uložené pakety

    // Získá seznam lokálních IP adres
    std::vector<std::string> getLocalIPAddresses();

    // Načte přesně daný počet bytů ze socketu
    bool recvExact(int socket, uint8_t* buffer, size_t len);
};

#endif // NETWORK_MANAGER_HPP
