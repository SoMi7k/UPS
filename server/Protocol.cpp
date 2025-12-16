#include "Protocol.hpp"
#include <iostream>
#include <sstream>

namespace Protocol {
    std::vector<uint8_t> serialize(const Message& msg) {
        std::vector<uint8_t> buffer;
        buffer.reserve(msg.size);

        // size
        buffer.push_back(msg.size & 0xFF);
        buffer.push_back((msg.size >> 8) & 0xFF);

        // packetID
        buffer.push_back(msg.packetID & 0xFF);

        // clientID
        buffer.push_back(msg.clientID & 0xFF);

        // type
        buffer.push_back(static_cast<uint8_t>(msg.type));

        // payload
        for (size_t i = 0; i < msg.fields.size(); ++i) {
            buffer.insert(buffer.end(),
                          msg.fields[i].begin(),
                          msg.fields[i].end());
            if (i + 1 < msg.fields.size()) {
                buffer.push_back('|');
            }
        }

        return buffer;
    }

    Message deserialize(const std::vector<uint8_t>& data) {
        Message msg;

        if (data.size() < HEADER_SIZE) {
            std::cerr << "❌ [PROTOCOL] Příliš malá zpráva: "
                      << data.size() << " bytů" << std::endl;
            return msg;
        }

        msg.size     = data[0] | (data[1] << 8);
        msg.packetID = data[2];
        msg.clientID = data[3];
        msg.type     = static_cast<MessageType>(data[4]);

        if (data.size() > HEADER_SIZE) {
            std::string payload(
                data.begin() + HEADER_SIZE,
                data.end()
            );

            std::stringstream ss(payload);
            std::string field;

            while (std::getline(ss, field, '|')) {
                msg.fields.push_back(field);
            }
        }

        return msg;
    }

    Message createMessage(int packetID, int clientID, MessageType type, const std::vector<std::string>& fields) {
        return Message(packetID, clientID, type, fields);
    }
}
