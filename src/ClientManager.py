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
    RECONNECT = "RECONNECT"
    READY = "READY"
    CARD = "CARD"
    TRICK = "TRICK"
    BIDDING = "BIDDING"
    RESET = "RESET"
    
    HEARTBEAT = "HEARTBEAT"
    OK = "OK"
    
class ClientManager:
    def __init__(self):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.settimeout(1.0)  # 1s timeout pro non-blocking
        
        # General variables
        self.connected = False
        self.on_message = None
        self.on_disconnect = None
        self.on_reconnecting = None  # 🆕 Callback pro UI
        self.on_reconnected = None   # 🆕 Callback při úspěšném reconnectu
        
        # Thread pro listening
        self.listen_thread = None
        self.running = False
        
        # Client info
        self.number = None      # Číslo hráče (0-2) z WELCOME
        self.nickname = None    # Jméno hráče (Player) z WELCOME
        
        # Message queue
        self.msg_queue = Queue()
        self.msg_processing_thread = None
        
        # 🆕 Reconnect konfigurace
        self.server_ip = None
        self.server_port = None
        self.auto_reconnect = False
        self.reconnect_thread = None
        self.reconnect_attempts = 0
        self.max_reconnect_attempts = 60  # 60 pokusů = ~1 minuta (1s pauza mezi pokusy)
        self.reconnect_delay = 1.0  # Sekund mezi pokusy
        self.is_reconnecting = False
        
    def connect(self, ip: str, port: int, reconnect: bool = False, auto_reconnect: bool = True) -> bool:
        """Připojí se k serveru (nebo se znovu připojí)."""
        try:
            # Uložíme server info pro auto-reconnect
            self.server_ip = ip
            self.server_port = port
            self.auto_reconnect = auto_reconnect
            
            # Pokud existuje staré připojení, zavři ho
            if hasattr(self, 'sock') and self.sock:
                try:
                    self.sock.close()
                except Exception as e:
                    print(f"❌ Chyba zavření starého socketu: {e}")
            
            # Vytvoř nový socket
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.sock.settimeout(1.0)
            
            print(f"Připojuji se na {ip}:{port}...")
            self.sock.connect((ip, port))
            self.connected = True
            self.is_reconnecting = False
            self.reconnect_attempts = 0
            
            # Spustí listening thread
            self.running = True
            self.listen_thread = threading.Thread(target=self._listen_loop, daemon=True)
            self.listen_thread.start()
            
            # Spustí vlákno pro zpracování zpráv
            self.msg_processing_thread = threading.Thread(target=self._process_message_queue, daemon=True)
            self.msg_processing_thread.start()
            
            # Pošli CONNECT/STATUS s nickname
            if self.nickname:
                if reconnect:
                    # Reconnect - pošleme RECONNECT s nickname jako session ID
                    self.send_message(MessageType.RECONNECT, {
                        "nickname": f"{self.nickname}",
                    })
                    print(f"🔄 Pokus o reconnect s session ID: {self.nickname}")
                else:
                    # První připojení
                    self.send_message(MessageType.CONNECT, {
                        "nickname": f"{self.nickname}",
                    })
                    
            # Start heartbeat
            threading.Thread(target=self._heartbeat_loop, daemon=True).start()
            
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
            print(f"📤 Odesílám: {msg_type.value}")
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
                json_str = self.msg_queue.get(timeout=0.1)
                
                print(f"⚙️ Zpracovávám zprávu z fronty (zbývá: {self.msg_queue.qsize()})")
                
                # Zpracování zprávy
                self._handle_message(json_str)
                
                # Označíme zprávu jako zpracovanou
                self.msg_queue.task_done()

            except Exception as _:
                # Timeout - pokračujeme
                continue
        
        print("🛑 Thread pro zpracování fronty ukončen")
    
    def _listen_loop(self):
        """Naslouchání a vkládání zpráv do fronty."""
        buffer = ""
        while self.running and self.connected:
            try:
                chunk = self.sock.recv(1024).decode('utf-8')
                if not chunk:
                    print("⚠️ Server uzavřel spojení")
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
                break
        
        # Spojení ztraceno
        self._handle_connection_lost()
    
    def _handle_connection_lost(self):
        """Zpracuje ztrátu spojení."""
        print("🔌 Spojení ztraceno!")
        
        was_connected = self.connected
        self.connected = False
        self.running = False
        
        # Zavřeme socket
        try:
            if self.sock:
                self.sock.close()
        except Exception as _:
            pass
        
        # Pokud máme povolený auto-reconnect A měli jsme nickname (tzn. byli jsme ve hře)
        if self.auto_reconnect and self.nickname and was_connected:
            print("🔄 Spouštím auto-reconnect...")
            self._start_reconnect()
        else:
            # Zavoláme disconnect callback
            if self.on_disconnect:
                self.on_disconnect()
    
    def _start_reconnect(self):
        """Spustí reconnect thread."""
        if self.is_reconnecting:
            return  # Už běží
        
        self.is_reconnecting = True
        self.reconnect_attempts = 0
        
        # Notifikujeme UI
        if self.on_reconnecting:
            self.on_reconnecting()
        
        self.reconnect_thread = threading.Thread(target=self._reconnect_loop, daemon=True)
        self.reconnect_thread.start()
    
    def _reconnect_loop(self):
        """Pokouší se znovu připojit."""
        print(f"🔄 Začínám reconnect (max {self.max_reconnect_attempts} pokusů)...")
        
        while self.is_reconnecting and self.reconnect_attempts < self.max_reconnect_attempts:
            self.reconnect_attempts += 1
            
            print(f"🔄 Pokus o reconnect {self.reconnect_attempts}/{self.max_reconnect_attempts}...")
            
            # Notifikace UI o pokusu
            if self.on_reconnecting:
                self.on_reconnecting(self.reconnect_attempts, self.max_reconnect_attempts)
            
            # Pokus o připojení
            success = self.connect(
                self.server_ip, 
                self.server_port, 
                reconnect=True,  # Řekneme, že jde o reconnect
                auto_reconnect=True
            )
            
            if success:
                print(f"✅ Reconnect úspěšný po {self.reconnect_attempts} pokusech!")
                self.is_reconnecting = False
                
                # Notifikace UI
                if self.on_reconnected:
                    self.on_reconnected()
                
                return
            
            # Čekáme před dalším pokusem
            time.sleep(self.reconnect_delay)
        
        # Vyčerpány pokusy
        print(f"❌ Reconnect selhal po {self.max_reconnect_attempts} pokusech")
        self.is_reconnecting = False
        
        # Zavoláme disconnect callback
        if self.on_disconnect:
            self.on_disconnect()
    
    def stop_reconnect(self):
        """Zastaví reconnect process."""
        self.is_reconnecting = False
        self.auto_reconnect = False
    
    def _handle_message(self, json_str: str):
        """Zpracuje přijatou zprávu."""
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
            self.send_message(MessageType.HEARTBEAT, {})
    
    def disconnect(self, stop_auto_reconnect: bool = True):
        """Bezpečné odpojení."""
        print("🔌 Manuální odpojení...")
        
        # ⚠️ KRITICKÉ: Nastavit flagy PŘED odesláním zprávy
        # Jinak listen thread může stihnout detekovat odpojení
        # a spustit reconnect před tím, než se zakáže
        if stop_auto_reconnect:
            self.stop_reconnect()
        
        self.running = False
        self.connected = False
        
        # Teprve teď posíláme DISCONNECT
        if self.sock:
            try:
                self.send_message(MessageType.DISCONNECT, {})
            except Exception as _:
                pass
    
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
        
    def send_empty_trick(self):
        self.send_message(MessageType.TRICK, {})