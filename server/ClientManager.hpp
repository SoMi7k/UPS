#ifndef CLIENT_MANAGER_HPP
#define CLIENT_MANAGER_HPP

#include <vector>
#include <mutex>
#include <string>
#include <thread>
#include <chrono>

#include "Protocol.hpp"

struct ClientInfo {
    int socket;
    int playerNumber;
    std::string address;
    bool connected;
    std::thread clientThread;
    std::chrono::steady_clock::time_point lastSeen;
    bool isDisconnected;
    std::string nickname;
    bool approved;
};

class NetworkManager;

class ClientManager {
public:
    ClientManager(int requiredPlayers, NetworkManager* networkManager);
    ~ClientManager();

    // Správa klientů
    ClientInfo* addClient(int socket, const std::string& address);
    void removeClient(ClientInfo* client);
    ClientInfo* findClientBySocket(int socket);
    ClientInfo* findClientByPlayerNumber(int playerNumber);
    void disconnectAll();
    void disconnectClient(ClientInfo* client);

    // Reconnect
    ClientInfo* findDisconnectedClient(const std::string& nickname);
    bool reconnectClient(ClientInfo* oldClient, int newSocket);
    void handleClientDisconnection(ClientInfo* client);
    void checkDisconnectedClients(bool running);

    // Packets
    void sendLossPackets(ClientInfo* client, int packetID);
    u_int8_t findPacketID(int clientNumber, int position);

    // Gettery
    int getConnectedCount() const;
    int getActiveCount() const;
    std::vector<ClientInfo*> getClients();
    void addPlayer() { connectedPlayers++ ;}

    // Zprávy
    void broadcastMessage(Protocol::MessageType msgType, std::vector<std::string> msg);
    void sendToPlayer(int playerNumber, Protocol::MessageType msgType, std::vector<std::string> msg);

    // Synchronizace jména
    int getreadyCount() const { return readyCount; };
    void setreadyCount() { readyCount++; };
    void nullreadyCount() { readyCount = 0; };

private:
    static constexpr int HEARTBEAT_TIMEOUT_SECONDS = 15;
    static constexpr int RECONNECT_TIMEOUT_SECONDS = 60;
    std::vector<ClientInfo*> clients;
    std::mutex clientsMutex;
    int requiredPlayers;
    int connectedPlayers;
    NetworkManager* networkManager;
    std::vector<int> clientNumbers;
    int readyCount = 0;

    int getFreeNumber();
};

#endif // CLIENT_MANAGER_HPP