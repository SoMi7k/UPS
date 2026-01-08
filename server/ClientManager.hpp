#ifndef CLIENT_MANAGER_HPP
#define CLIENT_MANAGER_HPP

#include <vector>
#include <mutex>
#include <string>
#include <thread>
#include <chrono>

#include "Protocol.hpp"

struct ClientInfo {
    int socket;                 // Socket klienta
    int playerNumber;           // Číslo hráče přidělené serverem
    std::string address;        // IP adresa klienta
    bool connected;             // Stav připojení klienta
    std::thread clientThread;   // Vlákno obsluhující klienta
    std::chrono::steady_clock::time_point lastSeen; // Čas poslední aktivity klienta
    bool isDisconnected;        // Příznak, zda byl klient odpojen
    std::string nickname;       // Přezdívka hráče
    bool approved;              // Schválení připojení (např. po reconnectu)
};

class NetworkManager;

class ClientManager {
public:
    ClientManager(int requiredPlayers, NetworkManager* networkManager);
    ~ClientManager();

    // Správa klientů
    ClientInfo* addClient(int socket, const std::string& address); // Přidá klinta do lobby
    void removeClient(ClientInfo* client); // Odstraní klienta ze serveru
    ClientInfo* findClientBySocket(int socket); // Najde clienta podle socketu
    ClientInfo* findClientByPlayerNumber(int playerNumber); // Nalezne klienta podle identifikačního čísla
    void disconnectAll(); // Odpojí všechny klienty
    void disconnectClient(ClientInfo* client); // Odpojí konkrétního klienta

    // Reconnect
    ClientInfo* findDisconnectedClient(const std::string& nickname); // Nalezne klienta, kterému spadl socket
    bool reconnectClient(ClientInfo* oldClient, int newSocket); // Provede recoonect, neboli obnovení klienta zpšt do hry
    void handleClientDisconnection(ClientInfo* client); // Řeší odpojení klienta v případě selhání socketu
    void checkDisconnectedClients(bool running); // Zjistí, jestli se čeká na recconect nějakého klienta

    // Packets
    void sendLossPackets(ClientInfo* client, int packetID); // Pošle klientovi zmenškané packety
    u_int8_t findPacketID(int clientNumber, int position); // Nalzenze packety podle ID packetů

    // Gettery
    int getConnectedCount() const; // Vrátí počet připojených hráčů (hráč může být v recconectu)
    int getActiveCount() const; // Vrátí počet všech aktivních hráčů (plně funkční sockety)
    std::vector<ClientInfo*> getClients();

    // Zprávy
    void broadcastMessage(Protocol::MessageType msgType, std::vector<std::string> msg); // Pošle zprávu všem klientům
    void sendToPlayer(int playerNumber, Protocol::MessageType msgType, std::vector<std::string> msg);  // Pošle zprávu konkrétnímu klientovi

    // Synchronizace jména
    int getreadyCount() const { return readyCount; };
    void setreadyCount() { readyCount++; };
    void nullreadyCount() { readyCount = 0; };

private:
    NetworkManager* networkManager;
    static constexpr int RECONNECT_TIMEOUT_SECONDS = 60;
    static constexpr int HEARTBEAT_TIMEOUT_SECONDS = 15;
    std::vector<ClientInfo*> clients;   // Pole připojených klientů
    std::mutex clientsMutex;            // Zámek pro přístup ke správě klientů
    int requiredPlayers;                // Pož. počet hráčů
    int connectedPlayers;               // Počet připojených hráčů
    std::vector<int> clientNumbers;     // Pole čísel pro inicializaci hráčů
    int readyCount = 0;                 // Pokud je hráčovo jméno v pořádku, je připraven ke hře

    int getFreeNumber(); // Zjistí dostupné číslo pro inicializaci klienta do hry
};

#endif // CLIENT_MANAGER_HPP