#ifndef NETWORK_MANAGER_HPP
#define NETWORK_MANAGER_HPP

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

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
    bool sendMessage(int socket, int clientNumber, const std::string& msgType, const std::string& message);
    std::string receiveMessage(int socket);
    
    // JSON operace
    nlohmann::json deserialize(const std::string& msg);
    std::string serialize(const std::string& msgType, const nlohmann::json& data);

    // Packets
    nlohmann::json findPacketByID(int clientNumber, int packetID);
    int findLatestPacketID(int clientNumber);

    // Gettery
    int getServerSocket() const { return serverSocket; }
    int getPort() const { return port; }
    const std::vector<nlohmann::json>& getPackets() const { return packets; }
    int getCurrentPacketID() const { return packetID; }

private:
    std::string bindIP;
    int serverSocket;
    int port;
    int packetID;
    std::vector<nlohmann::json> packets;

    std::vector<std::string> getLocalIPAddresses();
};

#endif // NETWORK_MANAGER_HPP