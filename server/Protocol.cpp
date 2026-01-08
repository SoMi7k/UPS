#include "Protocol.hpp"
#include <sstream>
#include <iostream>
#include <iomanip>

namespace Protocol {

    std::string serialize(const Message& msg) {
        std::stringstream ss;

        // Formát: SIZE|PACKET|CLIENT|TYPE|FIELD1|FIELD2|...\n
        ss << static_cast<int>(msg.packetID) << DELIMITER
           << static_cast<int>(msg.clientID) << DELIMITER
           << static_cast<int>(msg.type);

        // Přidáme fields
        for (const auto& field : msg.fields) {
            ss << DELIMITER << field;
        }
        ss << TERMINATOR;

        std::string content = ss.str();

        // Nyní přidáme SIZE na začátek
        uint16_t total_size = content.length() + 6;  // +6 pro "SIZE|" (max 5 číslic + |)

        std::stringstream final;
        final << total_size << DELIMITER << content;

        return final.str();
    }

    Message deserialize(const std::string& data) {
        Message msg;

        if (data.empty()) {
            std::cerr << "❌ [PROTOCOL] Prázdná zpráva" << std::endl;
            return msg;
        }

        // Rozdělíme podle delimiteru
        std::vector<std::string> parts;
        std::stringstream ss(data);
        std::string token;

        while (std::getline(ss, token, DELIMITER)) {
            // Odstraníme \n z posledního tokenu
            if (!token.empty() && token.back() == TERMINATOR) {
                token.pop_back();
            }
            parts.push_back(token);
        }

        // Minimálně potřebujeme: SIZE|PACKET|CLIENT|TYPE
        if (parts.size() < 4) {
            std::cerr << "❌ [PROTOCOL] Neplatný počet částí: " << parts.size() << std::endl;
            return msg;
        }

        try {
            // Parsování hlavičky
            msg.size = std::stoi(parts[0]);
            msg.packetID = std::stoi(parts[1]);
            msg.clientID = std::stoi(parts[2]);
            msg.type = static_cast<MessageType>(std::stoi(parts[3]));

            // Zbylé části jsou fields
            for (size_t i = 4; i < parts.size(); i++) {
                msg.fields.push_back(parts[i]);
            }

        } catch (const std::exception& e) {
            std::cerr << "❌ [PROTOCOL] Chyba při parsování: " << e.what() << std::endl;
        }

        return msg;
    }

    Message createMessage(int packetID, int clientID, MessageType type,
                         const std::vector<std::string>& fields) {
        return Message(
            static_cast<uint8_t>(packetID),
            static_cast<uint8_t>(clientID),
            type,
            fields
        );
    }
}