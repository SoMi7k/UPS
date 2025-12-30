# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra
TARGET = marias.exe

# Directories
GAME_DIR = server/game
SERVER_DIR = server
BUILD_DIR = build

# Source files
SRCS = $(GAME_DIR)/Card.cpp \
       $(GAME_DIR)/Hand.cpp \
       $(GAME_DIR)/Player.cpp \
       $(GAME_DIR)/Deck.cpp \
       $(GAME_DIR)/GameLogic.cpp \
       $(GAME_DIR)/Game.cpp \
       $(SERVER_DIR)/NetworkManager.cpp \
       $(SERVER_DIR)/LobbyManager.cpp \
       $(SERVER_DIR)/Protocol.cpp \
       $(SERVER_DIR)/ClientManager.cpp \
       $(SERVER_DIR)/GameManager.cpp \
       $(SERVER_DIR)/MessageHandler.cpp \
       $(SERVER_DIR)/Server.cpp \
       $(SERVER_DIR)/Main.cpp

# Object files in build directory
OBJS = $(BUILD_DIR)/Card.o \
       $(BUILD_DIR)/Hand.o \
       $(BUILD_DIR)/Player.o \
       $(BUILD_DIR)/Deck.o \
       $(BUILD_DIR)/GameLogic.o \
       $(BUILD_DIR)/Game.o \
       $(BUILD_DIR)/NetworkManager.o \
	   $(BUILD_DIR)/LobbyManager.o \
       $(BUILD_DIR)/Protocol.o \
       $(BUILD_DIR)/ClientManager.o \
       $(BUILD_DIR)/GameManager.o \
       $(BUILD_DIR)/MessageHandler.o \
       $(BUILD_DIR)/Server.o \
       $(BUILD_DIR)/Main.o

# Default target
all: $(BUILD_DIR) $(TARGET)

# Create build directory if it doesn't exist
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Link object files to create executable
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(TARGET)

# Compile game_server files
$(BUILD_DIR)/%.o: $(GAME_DIR)/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Compile server files
$(BUILD_DIR)/%.o: $(SERVER_DIR)/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR) $(TARGET)

# Rebuild everything
rebuild: clean all

# Phony targets
.PHONY: all clean rebuild
