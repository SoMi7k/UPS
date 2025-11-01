import socket
import json
import threading
import time
from enum import Enum
from typing import Dict, Any

class MessageType(Enum):
    # Server -> Client
    ERROR = "ERROR"
    STATUS = "STATUS"
    WELCOME = "WELCOME"
    STATE = "STATE"
    GAME_START = "GAME_START"
    DISCONNECT = "DISCONNECT"
    CLIENT_DATA = "CLIENT_DATA"
    YOUR_TURN = "YOUR_TURN"
    WAIT_LOBBY = "WAIT_LOBBY"
    WAIT = "WAIT"
    INVALID = "INVALID"
    # Client -> Server
    CONNECT = "CONNECT"
    READY = "READY"
    CARD = "CARD"
    BIDDING = "BIDDING"
    RESET = "RESET"
    
    HEARTBEAT = "HEARTBEAT"
    OK = "OK"
    
    
class Client:
    def __init__(self):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.settimeout(1.0)  # 1s timeout pro non-blocking
        
        self.connected = False
        self.on_message = None  # callback pro GUI
        self.on_disconnect = None
        
        # Thread pro listening
        self.listen_thread = None
        self.running = False
        
    def connect(self, ip: str, port: int) -> bool:
        """Připojí se k serveru a spustí listener."""
        try:
            print(f"Připojuji se na {ip}:{port}...")
            self.sock.connect((ip, port))
            self.connected = True
            
            # Spustí listening thread
            self.running = True
            self.listen_thread = threading.Thread(target=self._listen_loop, daemon=True)
            self.listen_thread.start()
            
            # Pošli CONNECT
            self.send_message(MessageType.CONNECT, {"nickname": "Player"})
            
            # Start heartbeat
            threading.Thread(target=self._heartbeat_loop, daemon=True).start()
            
            print("✅ Připojeno!")
            return True
            
        except Exception as e:
            print(f"❌ Chyba připojení: {e}")
            return False
    
    def send_message(self, msg_type: MessageType, data: Dict[str, Any]) -> bool|str:
        """Pošle zprávu (thread-safe)."""
        if not self.connected:
           return False
            
        try:
            message = {
                "type": msg_type.value,
                "data": data or {},
                "timestamp": time.time()
            }
            serialized = json.dumps(message) + "\n"
            self.sock.send(serialized.encode('utf-8'))
            return serialized
        except Exception as e:
            print(f"❌ Chyba odeslání: {e}")
            self.connected = False
            return False
    
    def _listen_loop(self):
        """NEPRERUŠITELNÉ naslouchání v samostatném vlákně."""
        buffer = ""
        while self.running and self.connected:
            try:
                chunk = self.sock.recv(1024).decode('utf-8')
                if not chunk:
                    break
                    
                buffer += chunk
                while "\n" in buffer:
                    line, buffer = buffer.split("\n", 1)
                    if line.strip():
                        self._handle_message(line)
                        
            except socket.timeout:
                continue  # Normální - timeout OK
            except Exception as e:
                print(f"❌ Listening chyba: {e}")
                self.connected = False
                break
        
        self._cleanup()
    
    def _handle_message(self, json_str: str):
        """Zpracuje přijatou zprávu a zavolá GUI callback."""
        try:
            msg = json.loads(json_str)
            msg_type = MessageType(msg["type"])
            
            if self.on_message:
                self.on_message(msg_type, msg.get("data", {}))
                
        except Exception as e:
            print(f"❌ Chyba parsování: {e}")
    
    def _heartbeat_loop(self):
        """Pošle heartbeat každých 10s."""
        while self.running and self.connected:
            time.sleep(10)
            self.sock.send(MessageType.HEARTBEAT)
    
    def _cleanup(self):
        """Uzavře připojení."""
        self.running = False
        self.connected = False
        if self.sock:
            self.sock.close()
        if self.on_disconnect:
            self.on_disconnect()
    
    def disconnect(self):
        """Bezpečné odpojení."""
        self.sock.send(MessageType.DISCONNECT)
        self._cleanup()