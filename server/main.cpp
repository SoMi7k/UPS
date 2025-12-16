#include "Server.hpp"
#include <iostream>
#include <csignal>
#include <cstring>
#include <regex>

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

void printUsage(const char* programName) {
    std::cout << "Použití: " << programName << " [volby]\n\n";
    std::cout << "Volby:\n";
    std::cout << "  -i IP        IP adresa serveru (výchozí: 0.0.0.0 = všechna rozhraní)\n";
    std::cout << "  -p PORT      Port serveru (výchozí: 10000)\n";
    std::cout << "  -l LOBBIES   Počet herních místností (výchozí: 1)\n";
    std::cout << "  -n PLAYERS   Počet hráčů na místnost (výchozí: 2)\n";
    std::cout << "  -h           Zobrazit tuto nápovědu\n\n";
    std::cout << "Příklady:\n";
    std::cout << "  " << programName << "                        # Výchozí nastavení (0.0.0.0:10000)\n";
    std::cout << "  " << programName << " -i 127.0.0.1           # Pouze localhost\n";
    std::cout << "  " << programName << " -i 192.168.1.100 -p 8080  # Konkrétní IP a port\n";
    std::cout << "  " << programName << " -p 9000 -l 2 -n 4      # 2 místnosti po 4 hráčích\n";
    std::cout << "\n";
    std::cout << "💡 Vysvětlení IP adres:\n";
    std::cout << "  0.0.0.0      - Naslouchá na VŠECH síťových rozhraních (LAN + localhost)\n";
    std::cout << "  127.0.0.1    - Pouze localhost (lokální připojení)\n";
    std::cout << "  192.168.x.x  - Konkrétní IP v lokální síti\n";
}

int main(int argc, char* argv[]) {
    // Výchozí hodnoty
    std::string ip = "0.0.0.0";  // 0.0.0.0 = naslouchá na všech rozhraních
    int port = 10000;
    int lobbies = 1;
    int players = 2;

    const std::regex ip_regex("^((25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$");

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-i") == 0 && i + 1 < argc) {
            std::string potential_ip = argv[i + 1];

            if (std::regex_match(potential_ip, ip_regex)) {
                ip = argv[++i];
                std::cout << "ℹ️  Nastavena IP adresa: " << ip << std::endl;
            } else {
                std::cout << "❌ Chyba: Hodnota '" << potential_ip << "' není platná IP adresa ve formátu xxx.xxx.xxx.xxx." << std::endl;
                i++;
                return 0;
            }
        }
        else if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
            try {
                port = std::stoi(argv[++i]);
                if (port < 1024 || port > 65535) {
                    std::cerr << "❌ Port musí být v rozsahu 1024-65535" << std::endl;
                    return 1;
                }
            } catch (...) {
                std::cerr << "❌ Neplatný port: " << argv[i] << std::endl;
                return 1;
            }
        }
        else if (strcmp(argv[i], "-l") == 0 && i + 1 < argc) {
            try {
                lobbies = std::stoi(argv[++i]);
                if (lobbies < 1 || lobbies > 100) {
                    std::cerr << "❌ Počet místností musí být 1-100" << std::endl;
                    return 1;
                }
            } catch (...) {
                std::cerr << "❌ Neplatný počet místností: " << argv[i] << std::endl;
                return 1;
            }
        }
        else if (strcmp(argv[i], "-n") == 0 && i + 1 < argc) {
            try {
                players = std::stoi(argv[++i]);
                if (players < 2 || players > 3) {
                    std::cerr << "❌ Počet hráčů musí být 2-3" << std::endl;
                    return 1;
                }
            } catch (...) {
                std::cerr << "❌ Neplatný počet hráčů: " << argv[i] << std::endl;
                return 1;
            }
        }
        else if (strcmp(argv[i], "-h") == 0) {
            printUsage(argv[0]);
            return 0;
        }
        else {
            std::cerr << "❌ Neznámý parametr: " << argv[i] << "\n" << std::endl;
            printUsage(argv[0]);
            return 1;
        }
    }

    // ASCII art header
    std::cout << "\n";
    std::cout << "╔════════════════════════════════════════╗\n";
    std::cout << "║                                        ║\n";
    std::cout << "║          🎮 MARIÁŠ SERVER 🎮          ║\n";
    std::cout << "║                                        ║\n";
    std::cout << "╚════════════════════════════════════════╝\n";
    std::cout << "\n";
    std::cout << "📋 KONFIGURACE:\n";
    std::cout << "   IP adresa:      " << ip << "\n";
    std::cout << "   Port:           " << port << "\n";
    std::cout << "   Místnosti:      " << lobbies << "\n";
    std::cout << "   Hráčů/místnost: " << players << "\n";
    std::cout << "   Celkem slotů:   " << (lobbies * players) << "\n";
    std::cout << "\n";

    // Vysvětlení IP adresy
    if (ip == "0.0.0.0") {
        std::cout << "🌐 Server naslouchá na VŠECH síťových rozhraních\n";
        std::cout << "   → Dostupný z lokální sítě i z localhost\n";
    } else if (ip == "127.0.0.1") {
        std::cout << "🏠 Server naslouchá pouze na LOCALHOST\n";
        std::cout << "   → Dostupný pouze lokálně (127.0.0.1)\n";
    } else {
        std::cout << "🎯 Server naslouchá na konkrétní IP: " << ip << "\n";
    }

    std::cout << "\n";
    std::cout << "💡 TIP: Pro nápovědu spusť s parametrem -h\n";
    std::cout << "\n";
    std::cout << std::string(44, '=') << "\n\n";

    // Vytvoříme server s IP adresou
    GameServer server(ip, port, players, lobbies);
    globalServer = &server;

    // Nastavíme signal handler pro Ctrl+C
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    try {
        // Spustíme server (blocking call)
        server.start();
    } catch (const std::exception& e) {
        std::cerr << "❌ Chyba serveru: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}