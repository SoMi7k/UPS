#include "LobbyManager.hpp"
#include "ClientManager.hpp"
#include "GameManager.hpp"
#include <iostream>

// ============================================================
// LOBBY - Implementace struktury pro jednu herní místnost
// ============================================================

Lobby::Lobby(int lobbyId, int players, NetworkManager *netManager)
    : id(lobbyId), gameStarted(false), requiredPlayers(players) {

  clientManager = std::make_unique<ClientManager>(players, netManager);
  gameManager =
      std::make_unique<GameManager>(players, netManager, clientManager.get());

  std::cout << "🏠 Lobby #" << id << " vytvořena (" << players << " hráčů)"
            << std::endl;
}

Lobby::~Lobby() {
  std::cout << "🗑️ Lobby #" << id << " destruktor" << std::endl;
}

int Lobby::getConnectedCount() const {
  return clientManager->getConnectedCount();
}

int Lobby::getActiveCount() const { return clientManager->getActiveCount(); }

bool Lobby::isFull() const { return getActiveCount() >= requiredPlayers; }

bool Lobby::canJoin() const {
  // Může se připojit, pokud není plná nebo pokud hra ještě nezačala
  return !isFull();
}

// ============================================================
// LOBBYMANAGER - Správce všech herních místností
// ============================================================

LobbyManager::LobbyManager(NetworkManager *netManager, int players,
                           int lobbyCount)
    : networkManager(netManager), requiredPlayers(players) {

  std::cout << "\n🏢 Vytvářím " << lobbyCount << " herních místností..."
            << std::endl;

  for (int i = 0; i < lobbyCount; i++) {
    lobbies.push_back(std::make_unique<Lobby>(i + 1, players, netManager));
  }

  std::cout << "✅ Všechny místnosti vytvořeny\n" << std::endl;
}

LobbyManager::~LobbyManager() {
  std::cout << "🗑️ LobbyManager destruktor" << std::endl;
  disconnectAll();
}

Lobby *LobbyManager::findAvailableLobby() {
  std::lock_guard<std::mutex> lock(lobbiesMutex);

  // Hledáme první volnou místnost
  for (auto &lobby : lobbies) {
    if (lobby->canJoin()) {
      return lobby.get();
    }
  }

  return nullptr;
}

Lobby *LobbyManager::getLobby(int lobbyId) {
  std::lock_guard<std::mutex> lock(lobbiesMutex);

  if (lobbyId < 1 || lobbyId > static_cast<int>(lobbies.size())) {
    return nullptr;
  }

  return lobbies[lobbyId - 1].get();
}

std::string LobbyManager::getLobbiesStatus() {
  std::lock_guard<std::mutex> lock(lobbiesMutex);

  std::string status = "\n📊 STAV MÍSTNOSTÍ:\n";
  status += std::string(40, '=') + "\n";

  for (const auto &lobby : lobbies) {
    status += "Lobby #" + std::to_string(lobby->id) + ": ";
    status += std::to_string(lobby->getConnectedCount()) + "/" +
              std::to_string(lobby->requiredPlayers);
    status += (lobby->gameStarted ? " (hra běží)" : " (čeká)");
    status += "\n";
  }

  status += std::string(40, '=') + "\n";
  return status;
}

void LobbyManager::disconnectAll() {
  std::lock_guard<std::mutex> lock(lobbiesMutex);

  std::cout << "🔌 Odpojuji všechny hráče ze všech místností..." << std::endl;

  for (auto &lobby : lobbies) {
    if (lobby->clientManager) {
      lobby->clientManager->disconnectAll();
    }
  }
}