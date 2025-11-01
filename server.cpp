// server.cpp
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <thread>
#include <string>
#include <random>
#include <vector>

#define HELLO "HELLO\n"
#define ERROR "ERROR\n"
#define OK "OK\n"
#define WRONG "WRONG\n"

/** Funkce přijímá číslo od uživatele do bufferu a převedeho na integer. **/
int getNumberFromMSG(const char* input, size_t len) {
    std::vector<int> cislo;

    for (size_t i = 0; i < len; i++) {
        const char digit = input[i];

        if (digit == '\n') break;
        if (digit == '\r') continue;

        if (std::isdigit(digit)) {
            cislo.push_back(digit - '0');
        } else {
            // Neplatný znak (není číslo)
            std::cout << "Neplatný znak v odpovědi" << std::endl;
            return -1;
        }
    }

    if (cislo.empty()) {
        std::cout << "Prázdná odpověď" << std::endl;
        return -1;
    }

    int result = 0;
    for (const int number : cislo) {
        result = result * 10 + number;
    }

    std::cout << "Obdržené číslo: " << result << std::endl;
    return result;
}

/** Hnadler pro klienta. Zaručí celý proces úkolu. **/
void handle_client(int clientSocket) {
    std::cout << "Klient " << clientSocket << " se připojil." << std::endl;

    char buffer[1024] = {0};

    // Přijme HELLO od klienta
    ssize_t n = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
    if (n <= 0) {
        std::cout << "Chyba při čtení HELLO" << std::endl;
        close(clientSocket);
        return;
    }
    buffer[n] = '\0';

    std::cout << "Message from client " << clientSocket << ": " << buffer;

    if (n < sizeof(HELLO) - 1 || std::memcmp(buffer, HELLO, sizeof(HELLO) - 1) != 0) {
        std::cout << "Neplatné HELLO od " << clientSocket << std::endl;
        send(clientSocket, ERROR, sizeof(ERROR) - 1, MSG_NOSIGNAL);
        close(clientSocket);
        return;
    }

    std::cout << "Přišlo HELLO od " << clientSocket << std::endl;

    // Vygeneruje náhodné číslo
    static thread_local std::mt19937 gen(std::random_device{}());
    std::uniform_int_distribution<> dis(0, 999999);
    const int random_number = dis(gen);

    const std::string num_msg = "NUM:" + std::to_string(random_number) + "\n";
    std::cout << "Odesílám: " << num_msg;

    if (send(clientSocket, num_msg.c_str(), num_msg.size(), MSG_NOSIGNAL) < 0) {
        std::cout << "Chyba při odesílání NUM" << std::endl;
        close(clientSocket);
        return;
    }

    // Přijme odpověď od klienta
    std::memset(buffer, 0, sizeof(buffer));
    n = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
    if (n <= 0) {
        std::cout << "Chyba při čtení odpovědi" << std::endl;
        send(clientSocket, ERROR, sizeof(ERROR) - 1, MSG_NOSIGNAL);
        close(clientSocket);
        return;
    }
    buffer[n] = '\0';

    std::cout << "Odpověď od klienta: " << buffer;

    // Ověření odpověďi
    const int check_sum = random_number * 2;
    const int client_answer = getNumberFromMSG(buffer, n);

    if (client_answer == -1) {
        // Neplatná odpověď (není číslo)
        send(clientSocket, ERROR, sizeof(ERROR) - 1, MSG_NOSIGNAL);
    } else if (client_answer == check_sum) {
        std::cout << "Správná odpověď!" << std::endl;
        send(clientSocket, OK, sizeof(OK) - 1, MSG_NOSIGNAL);
    } else {
        std::cout << "Špatná odpověď (očekáváno " << check_sum << ", dostáno " << client_answer << ")" << std::endl;
        send(clientSocket, WRONG, sizeof(WRONG) - 1, MSG_NOSIGNAL);
    }

    close(clientSocket);
    std::cout << "Klient " << clientSocket << " odpojen." << std::endl;
}

/** Otevření socketu, naslouchání od klienta. **/
int main() {
    // Vytvoření socketu
    const int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        std::cerr << "Chyba při vytváření socketu" << std::endl;
        return 1;
    }

    // Znovupoužití portu
    int opt = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // Nastavení adresy
    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(10000);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    // Bind
    if (bind(serverSocket, reinterpret_cast<struct sockaddr*>(&serverAddress), sizeof(serverAddress)) < 0) {
        std::cerr << "Chyba při bindování" << std::endl;
        close(serverSocket);
        return 1;
    }

    // Listen
    if (listen(serverSocket, 5) < 0) {
        std::cerr << "Chyba při naslouchání" << std::endl;
        close(serverSocket);
        return 1;
    }

    std::cout << "Server spuštěn na portu 10000" << std::endl;

    for (;;) {
        int clientSocket = accept(serverSocket, nullptr, nullptr);
        if (clientSocket < 0) {
            std::cerr << "Chyba při přijímání klienta" << std::endl;
            continue;
        }

        std::thread([clientSocket] {
            handle_client(clientSocket);
        }).detach();
    }

    close(serverSocket);
    return 0;
}