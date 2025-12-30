#ifndef PROTOCOL_HPP
#define PROTOCOL_HPP

#include <string>
#include <vector>
#include <cstdint>

// Struktura zprávy: [ Velikost (2B) | ID Packetu (1B) | Data (xB) ]
namespace Protocol {

    // Typy zpráv (1 byte)
    enum class MessageType : uint8_t {
        // Server -> Client
        ERROR = 0,
        STATUS = 1,
        WELCOME = 2,
        STATE = 3,
        GAME_START = 4,
        RESULT = 5,
        DISCONNECT = 6,
        CLIENT_DATA = 7,
        YOUR_TURN = 8,
        WAIT_LOBBY = 9,
        WAIT = 10,
        INVALID = 11,
        READY = 12,
        RECONNECT = 13,

        // Client -> Server
        CONNECT = 100,
        CARD = 101,
        TRICK = 102,
        BIDDING = 103,
        RESET = 104,
        HEARTBEAT = 105
    };

    // Maximální velikost zprávy (2 bajty = 65535)
    constexpr uint16_t MAX_MESSAGE_SIZE = 65535;
    constexpr uint8_t HEADER_SIZE = 5;  // 2B size + 1B clientID + 1B packetID + 1B type

    // Struktura zprávy
    struct Message {
        uint16_t size;
        uint8_t packetID;
        uint8_t clientID;
        MessageType type;
        std::vector<std::string> fields;

        Message() : size(0), packetID(0), clientID(0), type(MessageType::ERROR) {}

        Message(uint8_t id, uint8_t cid, MessageType t, const std::vector<std::string>& data)
            : packetID(id), clientID(cid), type(t), fields(data) {

            size = HEADER_SIZE;
            for (const auto& field : fields) {
                size += field.size();
            }
            if (!fields.empty()) {
                size += fields.size() - 1; // '|'
            }
        }
    };

    // Serializace zprávy do bytů
    std::vector<uint8_t> serialize(const Message& msg);

    // Deserializace bytů na zprávu
    Message deserialize(const std::vector<uint8_t>& data);

    // Helper funkce pro vytvoření zprávy
    Message createMessage(int packetID, int clientID, MessageType type, const std::vector<std::string>& fields);
}

#endif