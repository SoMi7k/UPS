#ifndef NETWORK_MANAGER_HPP
#define NETWORK_MANAGER_HPP

#include <string>
#include <vector>

#include "Protocol.hpp"

class NetworkManager {
public:
    NetworkManager(const std::string& ip, int port);
    ~NetworkManager();
    static constexpr int MAXIMUM_PACKET_SIZE = 255;

    // Socket operace
    bool initializeSocket();
    int acceptConnection();
    void closeSocket(int socket);
    void closeServerSocket();

    // Zprávy
    bool sendMessage(int socket, int clientNumber, Protocol::MessageType msgType, std::vector<std::string> msg);
    std::vector<uint8_t> receiveMessage(int socket);

    // Packets
    std::vector<u_int8_t> findPacketByID(int clientNumber, int packetID);
    int findLatestPacketID(int clientNumber);

    // Gettery
    int getServerSocket() const { return serverSocket; }
    int getPort() const { return port; }
    const std::vector<std::vector<u_int8_t>>& getPackets() const { return packets; }
    int getCurrentPacketID() const { return packetID; }

private:
    std::string bindIP;
    int serverSocket;
    int port;
    int packetID;
    std::vector<std::vector<u_int8_t>> packets;

    std::vector<std::string> getLocalIPAddresses();
    std::string debug_print_bytes(const std::vector<uint8_t>& buffer);
    bool recvExact(int socket, uint8_t* buffer, size_t len);
};

#endif // NETWORK_MANAGER_HPP