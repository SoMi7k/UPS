#ifndef GAME_SERVER_HPP
#define GAME_SERVER_HPP

#include "../game_server/Game.hpp"
#include <string>
#include <vector>
#include <mutex>
#include <thread>
#include <memory>
#include <nlohmann/json.hpp>

// Konstanty pro komunikační protokol
namespace messageType {
    // Client -> Server
    const std::string CONNECT = "CONNECT";
    const std::string READY = "READY";
    const std::string CARD = "CARD";
    const std::string BIDDING = "BIDDING";
    const std::string RESET = "RESET";
    const std::string HEARTBEAT = "HEARTBEAT";

    // Server -> Client
    const std::string ERROR = "ERROR";
    const std::string STATUS = "STATUS";
    const std::string WELCOME = "WELCOME";
    const std::string STATE = "STATE";
    const std::string DISCONNECT = "DISCONNECT";
    const std::string GAME_START = "GAME_START";
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
};

// Hlavní třída serveru
class GameServer {
private:
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

    // ============================================================
    // PRIVÁTNÍ METODY - Síťové operace
    // ============================================================

    /**
     * Inicializuje socket serveru (socket, bind, listen).
     * @return true pokud úspěch, false při chybě
     */
    bool initializeSocket();

    /**
     * Hlavní smyčka pro přijímání nových klientů (běží v samostatném vlákně).
     * Volá se z start().
     */
    void acceptClients();

    /**
     * Obsluha jednoho klienta (běží v samostatném vlákně pro každého klienta).
     * @param client Ukazatel na ClientInfo strukturu
     */
    void handleClient(ClientInfo* client);

    // ============================================================
    // PRIVÁTNÍ METODY - Komunikace s klienty
    // ============================================================

    /**
     * Pošle zprávu jednomu klientovi.
     * @param socket Socket klienta
     * @param msgType Typ zprávy (z messageType)
     * @param message Tělo zprávy (JSON string nebo prázdný string)
     * @return true pokud úspěch
     */
    bool sendMessage(int socket, const std::string& msgType, const std::string& message);

    /**
     * Pošle zprávu všem připojeným klientům.
     * @param msgType Typ zprávy
     * @param message Tělo zprávy
     */
    void broadcastMessage(const std::string& msgType, const std::string& message);

    /**
     * Pošle zprávu konkrétnímu hráči podle čísla.
     * @param playerNumber Číslo hráče (0-2)
     * @param msgType Typ zprávy
     * @param message Tělo zprávy
     */
    void sendToPlayer(int playerNumber, const std::string& msgType, const std::string& message);

    /**
     * Přijme zprávu od klienta (blokující volání).
     * @param socket Socket klienta
     * @return Přijatá zpráva (bez \n), nebo prázdný string při chybě
     */
    std::string receiveMessage(int socket);

    /**
     * Deserializuje JSON zprávu.
     * @param msg JSON string
     * @return Parsovaný JSON objekt
     */
    nlohmann::json deserialize(const std::string& msg);

    // ============================================================
    // PRIVÁTNÍ METODY - Zpracování zpráv
    // ============================================================

    /**
     * Zpracuje zprávu od klienta podle typu.
     * @param client Klient, který poslal zprávu
     * @param message Přijatá zpráva (JSON string)
     */
    void processClientMessage(ClientInfo* client, const std::string& message);

    // ============================================================
    // PRIVÁTNÍ METODY - Herní logika
    // ============================================================

    /**
     * Spustí hru (volá se když jsou všichni hráči připojeni).
     * Pošle GAME_START, počká 5s, rozdá karty, pošle CLIENT_DATA.
     */
    void startGame();

    /**
     * Pošle aktuální stav hry všem hráčům.
     */
    void sendGameState();

    /**
     * Pošle aktuální stav hry konkrétnímu hráči.
     * @param playerNumber Číslo hráče
     */
    void sendGameStateToPlayer(int playerNumber);

    /**
     * Notifikuje aktivního hráče, že je na tahu (pošle YOUR_TURN).
     */
    void notifyActivePlayer();

    // ============================================================
    // PRIVÁTNÍ METODY - Pomocné funkce
    // ============================================================

    /**
     * Odpojí klienta (zavře socket, označí jako odpojeného, odstraní ze seznamu).
     * @param client Klient k odpojení
     */
    void disconnectClient(ClientInfo* client);

    /**
     * Provede úklid (zavře všechny sockety, ukončí vlákna).
     */
    void cleanup();

    // ============================================================
    // PRIVÁTNÍ METODY - Serializace
    // ============================================================


    nlohmann::json serializeGameStart(int playerNumber);
    /**
     * Serializuje aktuální stav hry do JSON.
     * @return JSON objekt se stavem hry
     */
    nlohmann::json serializeGameState();

    /**
     * Serializuje data konkrétního hráče (ruku, body, atd.).
     * @param playerNumber Číslo hráče
     * @return JSON objekt s daty hráče
     */
    nlohmann::json serializePlayer(int playerNumber);

    /**
     * Serializuje jednu kartu do JSON.
     * @param card Karta k serializaci
     * @return JSON reprezentace karty
     */
    nlohmann::json serializeCard(const Card& card);

public:
    // ============================================================
    // VEŘEJNÉ METODY
    // ============================================================

    /**
     * Konstruktor serveru.
     * @param port Port, na kterém server poběží (výchozí 10000)
     */
    GameServer(int port = 10000);

    /**
     * Destruktor - provede cleanup.
     */
    ~GameServer();

    /**
     * Spustí server (vytvoří socket, spustí accept thread).
     * Blokující volání - běží dokud není zavolán stop().
     */
    void start();

    /**
     * Zastaví server (ukončí accept loop, odpojí všechny klienty).
     */
    void stop();

    /**
     * Vrátí zda server běží.
     * @return true pokud běží
     */
    bool isRunning() const;

    // ============================================================
    // GETTERY
    // ============================================================

    /**
     * Vrátí počet aktuálně připojených hráčů.
     * @return Počet připojených hráčů
     */
    int getConnectedPlayers() const;

    /**
     * Vrátí počet hráčů potřebných pro start hry.
     * @return Potřebný počet hráčů (3)
     */
    int getRequiredPlayers() const;
};

#endif // GAME_SERVER_HPP