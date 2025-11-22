#include "Server.hpp"
#include <iostream>
#include <csignal>

// Globální ukazatel na server pro signal handler
GameServer* globalServer = nullptr;

// Handler pro Ctrl+C (SIGINT)
void signalHandler(int signum) {
    std::cout << "\nPřijat signal " << signum << ", ukončuji server..." << std::endl;
    
    if (globalServer) {
        globalServer->stop();
    }
    
    exit(signum);
}

int main(int argc, char* argv[]) {
    // Výchozí port
    int port = 10000;
    
    // Pokud je zadán port jako argument
    if (argc > 1) {
        try {
            port = std::stoi(argv[1]);
        } catch (...) {
            std::cerr << "Neplatný port: " << argv[1] << std::endl;
            return 1;
        }
    }
    
    std::cout << "==================================" << std::endl;
    std::cout << "  Mariáš Server" << std::endl;
    std::cout << "==================================" << std::endl;
    std::cout << "Port: " << port << std::endl;
    std::cout << "Počet hráčů: 2" << std::endl;
    std::cout << "==================================" << std::endl;
    std::cout << std::endl;


    
    // Vytvoříme server
    GameServer server(port);
    globalServer = &server;
    
    // Nastavíme signal handler pro Ctrl+C
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    try {
        // Spustíme server (blocking call)
        server.start();
    } catch (const std::exception& e) {
        std::cerr << "Chyba serveru: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}