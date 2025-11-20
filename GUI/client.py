import socket
import json
import threading
import time
from enum import Enum
from typing import Dict, Any
from queue import Queue

class MessageType(Enum):
    # Server -> Client
    ERROR = "ERROR"
    STATUS = "STATUS"
    WELCOME = "WELCOME"
    STATE = "STATE"
    GAME_START = "GAME_START"
    RESULT = "RESULT"
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
    TRICK = "TRICK"
    BIDDING = "BIDDING"
    RESET = "RESET"
    
    HEARTBEAT = "HEARTBEAT"
    OK = "OK"
    
    
class Client:
    def __init__(self):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.settimeout(1.0)  # 1s timeout pro non-blocking
        
        # Gneneral variables
        self.connected = False
        self.on_message = None  # callback pro GUI
        self.on_disconnect = None
        
        # Thread pro listening
        self.listen_thread = None
        self.running = False
        
        # Client info
        self.session_id = None
        
        # Message queue
        self.msg_queue = Queue()
        self.msg_processing_thread = None
        
    def connect(self, ip: str, port: int, reconnect: bool = False) -> bool:
        """Připojí se k serveru (nebo se znovu připojí)."""
        try:
            # Pokud existuje staré připojení, zavři ho
            if hasattr(self, 'sock') and self.sock:
                try:
                    self.sock.close()
                except:
                    pass
            
            # Vytvoř nový socket
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.sock.settimeout(1.0)
            
            print(f"Připojuji se na {ip}:{port}...")
            self.sock.connect((ip, port))
            self.connected = True
            
            # Spustí listening thread
            self.running = True
            self.listen_thread = threading.Thread(target=self._listen_loop, daemon=True)
            self.listen_thread.start()
            
            # Spustí vlákno pro zpracování zpráv
            self.msg_processing_thread = threading.Thread(target=self._process_message_queue, daemon=True)
            self.msg_processing_thread.start()
            
            # Pošli CONNECT s session ID pokud reconnect
            if reconnect and self.session_id:
                self.send_message(MessageType.CONNECT, {
                    "nickname": "Player",
                    "sessionId": self.session_id
                })
                print(f"🔄 Pokus o reconnect se session ID: {self.session_id}")
            else:
                self.send_message(MessageType.CONNECT, {"nickname": "Player"})
            
            # Start heartbeat
            # threading.Thread(target=self._heartbeat_loop, daemon=True).start()
            
            print("✅ Připojeno!")
            return True
            
        except Exception as e:
            print(f"❌ Chyba připojení: {e}")
            self.connected = False
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
            print(f"Odeslaná zpráva ve formátu json - {serialized}")
            self.sock.send(serialized.encode('utf-8'))
            return serialized
        except Exception as e:
            print(f"❌ Chyba odeslání: {e}")
            self.connected = False
            return False
        
    def _process_message_queue(self):
        """Thread pro zpracování zpráv z fronty (jedna po druhé)."""
        print("🔄 Spuštěn thread pro zpracování fronty zpráv")
        while self.running:
            try:
                # Čekáme na zprávu (blokující s timeoutem)
                json_str = self.msg_queue.get()
                
                print(f"⚙️ Zpracovávám zprávu z fronty (zbývá: {self.msg_queue.qsize()})")
                
                # Zpracování zprávy
                self._handle_message(json_str)
                
                # Označíme zprávu jako zpracovanou
                self.msg_queue.task_done()

            except Exception as e:
                print(f"❌ Chyba při výběru zprávy z fronty - {e}")
                # Timeout nebo jiná chyba - pokračujeme
        
        print("🛑 Thread pro zpracování fronty ukončen")
    
    def _listen_loop(self):
        """Naslouchání a vkládání zpráv do fronty."""
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
                        self.msg_queue.put(line)
                        print(f"📥 Zpráva přidána do fronty (velikost: {self.msg_queue.qsize()})")
                        
            except socket.timeout:
                continue
            except Exception as e:
                print(f"❌ Listening chyba: {e}")
                self.connected = False
                break
        
        self._cleanup()
    
    def _handle_message(self, json_str: str):
        """Zpracuje přijatou zprávu."""
        try:
            msg = json.loads(json_str)
            msg_type = MessageType(msg["type"])
            
            if msg_type == MessageType.WELCOME and "sessionId" in msg.get("data", {}):
                self.session_id = msg["data"]["sessionId"]
                print(f"💾 Uloženo session ID: {self.session_id}")
            
            if self.on_message:
                self.on_message(msg_type, msg.get("data", {}))
                
        except Exception as e:
            print(f"❌ Chyba parsování: {e}")
    
    def _heartbeat_loop(self):
        """Pošle heartbeat každých 10s."""
        while self.running and self.connected:
            time.sleep(10)
            self.send_message(MessageType.HEARTBEAT, {})
    
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
        self.send_message(MessageType.DISCONNECT, {})
        self._cleanup()
    
    def wait_for_queue_empty(self, timeout: float = 5.0):
        """Počká, až se zpracují všechny zprávy ve frontě."""
        try:
            print(f"⏳ Čekám na vyprázdnění fronty (velikost: {self.msg_queue.qsize()})")
            self.msg_queue.join()  # Čeká, až jsou všechny zprávy zpracované
            print("✅ Fronta prázdná")
            return True
        except Exception as e:
            print(f"⚠️ Timeout při čekání na frontu: {e}")
            return False