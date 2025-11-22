#ifndef NETWORK_MANAGER_HPP
#define NETWORK_MANAGER_HPP

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

class NetworkManager {
public:
    NetworkManager(int port);
    ~NetworkManager();

    // Socket operace
    bool initializeSocket();
    int acceptConnection();
    void closeSocket(int socket);
    void closeServerSocket();

    // Zprávy
    bool sendMessage(int socket, const std::string& msgType, const std::string& message);
    std::string receiveMessage(int socket);
    
    // JSON operace
    nlohmann::json deserialize(const std::string& msg);
    std::string serialize(const std::string& msgType, const nlohmann::json& data);

    // Gettery
    int getServerSocket() const { return serverSocket; }
    int getPort() const { return port; }

private:
    int serverSocket;
    int port;

    std::vector<std::string> getLocalIPAddresses();
};

#endif // NETWORK_MANAGER_HPP