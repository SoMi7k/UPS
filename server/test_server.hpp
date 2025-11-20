#ifndef GAME_SERVER_HPP
#define GAME_SERVER_HPP

#include "../game_server/Game.hpp"
#include <string>
#include <vector>
#include <mutex>
#include <thread>
#include <memory>
#include <nlohmann/json.hpp>
#include <condition_variable>

// Konstanty pro komunikační protokol
namespace messageType {
    // Client -> Server
    const std::string CONNECT = "CONNECT";
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

// Struktura reprezentující připojeného klienta
struct ClientInfo {
    int socket;                    // Socket klienta
    int playerNumber;              // Číslo hráče (0-2)
    std::string address;           // IP adresa klienta
    bool connected;                // Zda je připojen
    std::thread clientThread;      // Vlákno obsluhující klienta
    std::string sessionId;         // Unikátní ID pro reconnect
    std::chrono::steady_clock::time_point lastSeen;
    bool isDisconnected;           // Dočasně odpojený (ne úplně odstraněný)
    std::string nickname;          // Uložený nickname
};

// Hlavní třída serveru
class GameServer {
private:private:
    static constexpr int RECONNECT_TIMEOUT_SECONDS = 120; // 2 minuty na reconnect

    // ============================================================
    // RECONNECT FUNKCE
    // ============================================================
    std::string generateSessionId();
    ClientInfo* findDisconnectedClient(const std::string& sessionId);
    void checkDisconnectedClients(); // Kontrola timeoutů
    void handleClientDisconnection(ClientInfo* client);
    bool reconnectClient(ClientInfo* oldClient, int newSocket);

    // ============================================================
    // SÍŤOVÉ NASTAVENÍ
    // ============================================================
    int serverSocket;              // Hlavní socket serveru
    int port;                      // Port, na kterém běží
    bool running;                  // Zda server běží
    std::thread acceptThread;      // Vlákno pro přijímání klientů

    // ============================================================
    // HERNÍ STAV
    // ============================================================
    std::unique_ptr<Game> game;    // Instance hry
    std::mutex gameMutex;          // Mutex pro thread-safe přístup ke hře

    // ============================================================
    // PŘIPOJENÍ KLIENTI
    // ============================================================
    std::vector<ClientInfo*> clients;  // Seznam klientů
    std::mutex clientsMutex;           // Mutex pro přístup k seznamu klientů
    int connectedPlayers;              // Počet připojených hráčů
    const int requiredPlayers = 2;     // Potřebný počet hráčů
    std::mutex trickMutex;
    std::condition_variable trickCV;
    int trickResponses = 0;

    // ============================================================
    // PRIVÁTNÍ METODY - Síťové operace
    // ============================================================
    bool initializeSocket();
    void acceptClients();
    void handleClient(ClientInfo* client);

    // ============================================================
    // PRIVÁTNÍ METODY - Komunikace s klienty
    // ============================================================
    bool sendMessage(int socket, const std::string& msgType, const std::string& message);
    void broadcastMessage(const std::string& msgType, const std::string& message);
    void sendToPlayer(int playerNumber, const std::string& msgType, const std::string& message);
    std::string receiveMessage(int socket);
    nlohmann::json deserialize(const std::string& msg);

    // ============================================================
    // PRIVÁTNÍ METODY - Zpracování zpráv
    // ============================================================
    void processClientMessage(ClientInfo* client, const std::string& message);

    // ============================================================
    // PRIVÁTNÍ METODY - Herní logika
    // ============================================================
    void startGame();
    void sendGameStateToPlayer(int playerNumber);
    void sendInvalidPlayer(int playerNumber);
    void notifyActivePlayer();

    // ============================================================
    // PRIVÁTNÍ METODY - Pomocné funkce
    // ============================================================
    void disconnectClient(ClientInfo* client);
    void cleanup();

    // ============================================================
    // PRIVÁTNÍ METODY - Serializace
    // ============================================================
    nlohmann::json serializeGameStart(int playerNumber);
    nlohmann::json serializeGameState();
    nlohmann::json serializePlayer(int playerNumber);
    nlohmann::json serializeInvalid(int playerNumber);

public:
    // ============================================================
    // VEŘEJNÉ METODY
    // ============================================================
    GameServer(int port = 10000);
    ~GameServer();

    void start();
    void stop();
    bool isRunning() const;

    // ============================================================
    // GETTERY
    // ============================================================
    int getConnectedPlayers() const;
    int getRequiredPlayers() const;
};

#endif // GAME_SERVER_HPP