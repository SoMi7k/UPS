#ifndef CLIENT_MANAGER_HPP
#define CLIENT_MANAGER_HPP

#include <vector>
#include <mutex>
#include <string>
#include <thread>
#include <chrono>

// Konstanty pro komunikační protokol
namespace messageType {
    // Client -> Server
    const std::string CONNECT = "CONNECT";
    const std::string RECONNECT = "RECONNECT";
    const std::string READY = "READY";
    const std::string CARD = "CARD";
    const std::string BIDDING = "BIDDING";
    const std::string RESET = "RESET";
    const std::string HEARTBEAT = "HEARTBEAT";
    const std::string TRICK = "TRICK";

    // Server -> Client
    const std::string ERROR = "ERROR";
    const std::string STATUS = "STATUS";
    const std::string WELCOME = "WELCOME";
    const std::string STATE = "STATE";
    const std::string DISCONNECT = "DISCONNECT";
    const std::string GAME_START = "GAME_START";
    const std::string RESULT = "RESULT";
    const std::string CLIENT_DATA = "CLIENT_DATA";
    const std::string YOUR_TURN = "YOUR_TURN";
    const std::string WAIT = "WAIT";
    const std::string WAIT_LOBBY = "WAIT_LOBBY";
    const std::string INVALID = "INVALID";
    const std::string OK = "OK";
}

struct ClientInfo {
    int socket;
    int playerNumber;
    std::string address;
    bool connected;
    std::thread clientThread;
    std::chrono::steady_clock::time_point lastSeen;
    bool isDisconnected;
    std::string nickname;
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

    // Gettery
    int getConnectedCount() const;
    int getActiveCount() const;
    std::vector<ClientInfo*> getClients();

    // Zprávy
    void broadcastMessage(const std::string& msgType, const std::string& message);
    void sendToPlayer(int playerNumber, const std::string& msgType, const std::string& message);

    // Synchronizace jména
    int getreadyCount() const { return readyCount; };
    void setreadyCount() { readyCount++; };

private:
    static constexpr int RECONNECT_TIMEOUT_SECONDS = 30;

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