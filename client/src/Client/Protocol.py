import struct
from enum import IntEnum
from typing import List

# ============================================================
# PROTOCOL - Definice protokolu (stejné jako v C++)
# ============================================================
class MessageType(IntEnum):
    ERROR = 0
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
    READY = 12
    RECONNECT = 13
    CONNECT = 14
    CARD = 15
    TRICK = 16
    BIDDING = 17
    RESET = 18
    HEARTBEAT = 19

class Protocol:
    """Binární protokol: [ 2B Velikost | 1B Packet ID | 1B Client Number | 1B Type | Data (oddělené '|') ]"""
    
    HEADER_SIZE = 5  # 2B size + 1B packetID + 1B clientNum + 1B type
    MAX_MESSAGE_SIZE = 65535
    
    @staticmethod
    def serialize(packet_id: int, client_number: int, msg_type: MessageType, fields: List[str]) -> bytes:
        """Serializuje zprávu do binárního formátu."""
        
        # 1. Spojíme pole s delimiterem '|'
        payload = '|'.join(fields).encode('utf-8')
        
        # 2. Vypočítáme celkovou velikost
        total_size = Protocol.HEADER_SIZE + len(payload)
        
        if total_size > Protocol.MAX_MESSAGE_SIZE:
            raise ValueError(f"Zpráva příliš velká: {total_size} bytů")
        
        # 3. Sestavíme header
        # Format: '<H' = little-endian unsigned short (2B)
        #         'B'  = unsigned byte (1B)
        header = struct.pack('<HBBB', 
                                total_size,         # 2B: Velikost zprávy
                                packet_id,          # 1B: Packet ID
                                client_number,      # 1B: Client number
                                int(msg_type))      # 1B: Message type
        
        # 4. Spojíme header + payload
        return header + payload
    
    @staticmethod
    def deserialize(data: bytes) -> tuple:
        """Deserializuje binární zprávu.
        
        Returns:
            tuple: (size, packet_id, client_number, msg_type, fields)
        """
        if len(data) < Protocol.HEADER_SIZE:
            raise ValueError(f"Příliš malá zpráva: {len(data)} bytů")
        
        # 1. Parsujeme header
        size, packet_id, client_number, msg_type_raw = struct.unpack('<HBBB', data[:Protocol.HEADER_SIZE])
        
        # 2. Validace
        if size != len(data):
            raise ValueError(f"Nekonzistentní velikost: header={size}, skutečnost={len(data)}")
        
        msg_type = MessageType(msg_type_raw)
        
        # 3. Parsujeme payload (pokud existuje)
        fields = []
        if len(data) > Protocol.HEADER_SIZE:
            payload = data[Protocol.HEADER_SIZE:].decode('utf-8')
            if payload:
                fields = payload.split('|')
        
        return size, packet_id, client_number, msg_type, fields
    
    