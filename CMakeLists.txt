cmake_minimum_required(VERSION 3.22)
project(UPS)

# Nastavte standard C++ pro celý projekt
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True) # Zajišťuje, že kompilace selže, pokud C++14 není k dispozici

# 1. Definujte všechny zdrojové soubory
# Použijte file(GLOB ...) pro nalezení všech .cpp souborů.
# I když je file(GLOB) pro velké projekty nedoporučovaný, pro malý projekt je to nejjednodušší.
file(GLOB SERVER_SOURCES "server/*.cpp")
file(GLOB GAME_SOURCES "server/game/*.cpp")

# Spojte všechny .cpp soubory do jednoho seznamu
set(PROJECT_SOURCES ${SERVER_SOURCES}   ${GAME_SOURCES}
        server/GameManager.cpp
        server/LobbyManager.cpp
        server/LobbyManager.hpp
        server/Protocol.cpp
        server/Protocol.hpp)

# 2. Vytvořte jedinou spustitelnou aplikaci s názvem UPS
add_executable(UPS ${PROJECT_SOURCES})

# 3. Nastavte cesty pro hlavičkové soubory
# Toto je klíčový krok, který řeší váš problém s `#include`.
# Říká CMake, že při hledání HLAVIČKOVÝCH souborů má prohledávat KORENOVY adresář projektu.
# Tímto se povolí zápis `#include "game_server/Game.hpp"`
target_include_directories(UPS PUBLIC
        # Přidáme kořenový adresář projektu (kde je CMakeLists.txt)
        "${PROJECT_SOURCE_DIR}"
)

# Poznámka: Složky "server" a "game_server" nemusíme přidávat zvlášť,
# protože cesty "game_server/Game.hpp" jsou již relativní k "${PROJECT_SOURCE_DIR}".