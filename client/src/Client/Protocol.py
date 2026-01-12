from enum import IntEnum

# ============================================================
# PROTOCOL - Definice protokolu (stejné jako v C++)
# ============================================================
class MessageType(IntEnum):
    STATUS = 1
    WELCOME = 2
    STATE = 3
    GAME_START = 4
    RESULT = 5
    DISCONNECT = 6
    CLIENT_DATA = 7
    YOUR_TURN = 8
    WAIT_LOBBY = 9
    WAIT = 10
    INVALID = 11
    AUTHORIZE = 12
    RECONNECT = 13
    CONNECT = 14
    CARD = 15
    TRICK = 16
    BIDDING = 17
    RESET = 18
    PING = 19
    PONG = 20

DELIMITER = '|'
TERMINATOR = '\n'

class Protocol:
    """Binární protokol: [ 2B Velikost | 1B Packet ID | 1B Client Number | 1B Type | Data (oddělené '|') ]"""
    
    MAX_MESSAGE_SIZE = 256

    @staticmethod
    def serialize(packet_id: int, client_id: int, msg_type: MessageType, fields: list[str]) -> bytes:
        '''Serializuje zprávu do textového formátu.'''
        # Format: SIZE|PACKET|CLIENT|TYPE|FIELD1|FIELD2|...\n
        parts = [
            str(packet_id),
            str(client_id),
            str(msg_type.value)
        ] + fields
        
        content = DELIMITER.join(parts) + TERMINATOR
        size = len(content) + len(str(len(content))) + 1  # +1 pro delimiter za SIZE
        
        message = f"{size}{DELIMITER}{content}"
        return message.encode('utf-8')
    
    @staticmethod
    def deserialize(text: str):
        if not text:
            raise ValueError("Prázdná zpráva")

        text = text.strip()

        parts = text.split(DELIMITER)

        if len(parts) < 4:
            raise ValueError(f"Neplatná zpráva: {text}")

        packet_id = int(parts[1])
        client_id = int(parts[2])
        msg_type = MessageType(int(parts[3]))
        fields = parts[4:] if len(parts) > 4 else []

        return packet_id, client_id, msg_type, fields
    
    @staticmethod
    def is_valid_message_string(data: str) -> bool:
        if not data:
            print("Case 1")
            return False

        if len(data) > Protocol.MAX_MESSAGE_SIZE:
            print("Case 2")
            return False

        if not data.endswith('\n'):
            print("Case 3")
            return False

        if data.count(DELIMITER) < 2:
            print("Case 4")
            return False

        for c in data:
            if ord(c) == 0:
                print("Case 5")
                return False
            if ord(c) < 32 and c not in ['\n', '\r']:
                print("Case 6")
                return False

        size_part = data.split(DELIMITER, 1)[0]
        if not size_part.isdigit():
            print("Case 6")
            return False

        # Spam protection – 100+ stejných znaků
        last = None
        count = 0
        for c in data:
            if c == last:
                count += 1
                if count > 100:
                    return False
            else:
                last = c
                count = 1

        return True