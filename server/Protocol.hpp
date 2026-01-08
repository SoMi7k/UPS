#ifndef PROTOCOL_HPP
#define PROTOCOL_HPP

#include <string>
#include <vector>
#include <cstdint>
#include <sstream>

// Struktura zprávy: [ Velikost (2B) | ID Packetu (1B) | Data (xB) ]
namespace Protocol {

    // Typy zpráv (stejné jako předtím)
    enum class MessageType : uint8_t {
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
        CONNECT = 14,
        CARD = 15,
        TRICK = 16,
        BIDDING = 17,
        RESET = 18,
        HEARTBEAT = 19
    };

    // Konstanty
    constexpr char DELIMITER = '|';
    constexpr char TERMINATOR = '\n';
    constexpr uint16_t MAX_MESSAGE_SIZE = 65535;

    // Struktura zprávy
    struct Message {
        uint16_t size;          // Celková velikost
        uint8_t packetID;       // ID packetu
        uint8_t clientID;       // ID klienta
        MessageType type;       // Typ zprávy
        std::vector<std::string> fields;  // Data

        Message() : size(0), packetID(0), clientID(0), type(MessageType::ERROR) {}

        Message(uint8_t pID, uint8_t cID, MessageType t, const std::vector<std::string>& data)
            : packetID(pID), clientID(cID), type(t), fields(data) {

            // Vypočítáme velikost
            calculateSize();
        }

        void calculateSize() {
            // Format: SIZE|PACKET|CLIENT|TYPE|FIELD1|FIELD2|...\n
            std::stringstream ss;
            ss << "00000" << DELIMITER  // Placeholder pro SIZE (max 5 číslic)
               << static_cast<int>(packetID) << DELIMITER
               << static_cast<int>(clientID) << DELIMITER
               << static_cast<int>(type);

            for (const auto& field : fields) {
                ss << DELIMITER << field;
            }
            ss << TERMINATOR;

            size = ss.str().length();
        }
    };

    // Serializace zprávy do stringu
    std::string serialize(const Message& msg);

    // Deserializace stringu na zprávu
    Message deserialize(const std::string& data);

    // Helper funkce pro vytvoření zprávy
    Message createMessage(int packetID, int clientID, MessageType type,
                         const std::vector<std::string>& fields);
}

#endif