import socket
import struct
import threading
import time
from typing import List, Optional, Callable
from queue import Queue
from .Protocol import Protocol, MessageType
from src.View.GuiManager import GuiManager

# ============================================================
# CLIENT MANAGER - Síťová komunikace
# ============================================================
class ClientManager:
    def __init__(self, guiManager: GuiManager):
        self.sock: Optional[socket.socket] = None
        self.guiManager = guiManager
        
        # Connection state
        self.connected = False
        self.running = False
        
        # Callbacks
        self.on_message: Optional[Callable] = None
        self.on_disconnect: Optional[Callable] = None
        self.on_reconnecting: Optional[Callable] = None
        self.on_reconnected: Optional[Callable] = None
        
        # Client info
        self.number: Optional[int] = None
        self.nickname: Optional[str] = None
        self.last_packet_id: int = 0
        
        # Threads
        self.listen_thread: Optional[threading.Thread] = None
        self.msg_processing_thread: Optional[threading.Thread] = None
        self.msg_queue: Queue = Queue()
        
        # Reconnect config
        self.server_ip: Optional[str] = None
        self.server_port: Optional[int] = None
        self.auto_reconnect = False
        self.is_reconnecting = False
        self.reconnect_attempts = 0
        self.max_reconnect_attempts = 5
        self.reconnect_delay = 5.0
        
        self.msgCounter = 0
    
    def connect(self, ip: str, port: int, reconnect: bool = False, auto_reconnect: bool = True) -> bool:
        """Připojí se k serveru."""
        try:
            # Uložíme server info
            self.server_ip = ip
            self.server_port = port
            self.auto_reconnect = auto_reconnect
            
            # Zavřeme staré připojení
            if self.sock:
                try:
                    self.sock.close()
                except:
                    pass
            
            # Vytvoříme nový socket
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.sock.settimeout(1.0)
            
            print(f"Připojuji se na {ip}:{port}...")
            self.sock.connect((ip, port))
            
            self.connected = False
            self.is_reconnecting = False
            self.running = True
            
            # Spustíme listening thread
            self.listen_thread = threading.Thread(target=self._listen_loop, daemon=True)
            self.listen_thread.start()
            
            # Spustíme message processing thread
            self.msg_processing_thread = threading.Thread(target=self._process_message_queue, daemon=True)
            self.msg_processing_thread.start()
            
            # Spustíme heartbeat thread
            threading.Thread(target=self._heartbeat_loop, daemon=True).start()
            
            # Pošleme CONNECT nebo RECONNECT
            if self.nickname:
                if reconnect:
                    self.send_message(MessageType.RECONNECT, [self.nickname, str(self.last_packet_id)])
                    print(f"🔄 Pokus o reconnect: {self.nickname}")
                else:
                    self.send_message(MessageType.CONNECT, [self.nickname])
            
            return True
            
        except Exception as e:
            print(f"❌ Chyba připojení: {e}")
            self.connected = False
            return False
    
    def send_message(self, msg_type: MessageType, fields: List[str]) -> bool:
        """Pošle zprávu serveru."""
        try:
            if not self.sock:
                print("❌ Socket není inicializován")
                return False
            
            # Packet ID a client number (server je ignoruje, ale musíme je poslat)
            client_number = self.number if self.number is not None else 0
            
            # Serializujeme zprávu
            data = Protocol.serialize(self.last_packet_id, client_number, msg_type, fields)
            
            print(f"📤 Odesílám: {msg_type.name} ({len(data)} bytů)")
            print(f"📤 Odeslané byty: {data}")
            
            # Odešleme celou zprávu
            self._send_exactly(data)
            
            return True
            
        except Exception as e:
            print(f"❌ Chyba odeslání: {e}")
            self.connected = False
            return False
    
    def check_msg(self, msg: bytes, required_players: int) -> int:
        if (msg[2] < -1 and msg[2] > required_players - 1):
            return 0

        if (msg[3] < -1 or msg[3] > 19):
            return 0

        return 1
    
    def _send_exactly(self, data: bytes):
        """Pošle všechna data (ošetření částečného send)."""
        sent = 0
        while sent < len(data):
            try:
                n = self.sock.send(data[sent:])
                if n == 0:
                    raise ConnectionError("Socket uzavřen")
                sent += n
                
                if sent < len(data):
                    print(f"⏳ Částečné odeslání: {sent}/{len(data)} bytů")
                    
            except socket.timeout:
                continue
    
    def _recv_exactly(self, size: int) -> Optional[bytes]:
        """Načte přesně N bytů (ošetření částečného recv)."""
        data = b''
        while len(data) < size:
            try:
                chunk = self.sock.recv(size - len(data))
                if not chunk:
                    print("⚠️ Server uzavřel spojení")
                    return None
                data += chunk
                
                if len(data) < size:
                    print(f"⏳ Částečné čtení: {len(data)}/{size} bytů")
                    
            except socket.timeout:
                continue
            except Exception as e:
                print(f"❌ Chyba recv: {e}")
                return None
        
        return data
    
    def _listen_loop(self):
        """Naslouchá příchozím zprávám."""
        print("🔄 Listening thread spuštěn")
        
        while self.running:
            try:
                # 1. Načteme header (5 bytů)
                header = self._recv_exactly(Protocol.HEADER_SIZE)
                if not header:
                    break
                
                # 2. Parsujeme velikost zprávy
                msg_size = struct.unpack('<H', header[:2])[0]
                
                print(f"📥 Přijímám zprávu ({msg_size} bytů)...")
                
                # 3. Validace velikosti
                if msg_size < Protocol.HEADER_SIZE or msg_size > Protocol.MAX_MESSAGE_SIZE:
                    print(f"❌ Neplatná velikost zprávy: {msg_size}")
                    break
                
                # 4. Validace zprávy
                #if not self.check_msg(header ):
                #    print("❌ Neplatná zpráva")
                #    break
                
                # 5. Načteme zbytek zprávy (payload)
                payload_size = msg_size - Protocol.HEADER_SIZE
                full_message = header
                
                if payload_size > 0:
                    payload = self._recv_exactly(payload_size)
                    if not payload:
                        break
                    full_message += payload
                
                # 6. Přidáme do fronty
                self.msg_queue.put(full_message)
                print(f"📥 Zpráva přidána do fronty (velikost: {self.msg_queue.qsize()})")
                
            except socket.timeout:
                continue
            except Exception as e:
                print(f"❌ Listening chyba: {e}")
                break
        
        # Spojení ztraceno
        self._handle_connection_lost()
    
    def _process_message_queue(self):
        """Zpracovává zprávy z fronty."""
        print("🔄 Message processing thread spuštěn")
        
        while self.running:
            try:
                # Čekáme na zprávu
                data = self.msg_queue.get(timeout=0.1)
                
                print(f"⚙️ Zpracovávám zprávu z fronty (zbývá: {self.msg_queue.qsize()})")
                
                # Deserializujeme a zpracujeme
                self._handle_message(data)
                
                # Označíme jako hotovo
                self.msg_queue.task_done()
                
            except:
                continue
        
        print("🛑 Message processing thread ukončen")
    
    def _handle_message(self, data: bytes):
        """Zpracuje přijatou zprávu."""
        try:
            # Deserializujeme
            size, packet_id, _, msg_type, fields = Protocol.deserialize(data)
            
            self.last_packet_id = packet_id
            
            print(f"📨 Přijatá velikost: {size}")
            print(f"📨 Přijata zpráva: {msg_type.name}, pole: {len(fields)}")
            if fields:
                print(f"   Data: {fields}")
            
            # Speciální zpracování některých zpráv
            if msg_type == MessageType.DISCONNECT:
                self.connected = False
                if fields:
                    self.guiManager.error_message = fields[0]
                # 🆕 Zakážeme auto-reconnect při DISCONNECT
                self.auto_reconnect = False
                self.is_reconnecting = False
                
            elif msg_type in [MessageType.RECONNECT, MessageType.READY]:
                self.connected = True
                self.guiManager.error_message = ""
            
            # Zavoláme callback
            if self.on_message:
                self.on_message(msg_type, fields)
            
            self.msgCounter = 0
                
        except Exception as e:
            print(f"❌ Chyba parsování zprávy: {e}")
            self.msgCounter += 1
            if self.msgCounter == 3:
                self.guiManager.error_message = "Server posílá nesprávné zprávy!"
                self.disconnect()
                self.msgCounter = 0
    
    def _handle_connection_lost(self):
        """Zpracuje ztrátu spojení."""
        print("🔌 Spojení ztraceno!")
        
        was_connected = self.connected
        self.connected = False
        self.running = False
        
        # Zavřeme socket
        if self.sock:
            try:
                self.sock.close()
            except:
                pass
        
        # Kontrola auto-reconnect
        if not self.auto_reconnect:
            print("🚫 Auto-reconnect zakázán")
            if self.on_disconnect:
                self.on_disconnect()
            return
        
        # Spustíme reconnect
        if self.nickname and was_connected:
            print("🔄 Spouštím auto-reconnect...")
            self._start_reconnect()
        else:
            if self.on_disconnect:
                self.on_disconnect()
    
    def _start_reconnect(self):
        """Spustí reconnect thread."""
        if self.is_reconnecting:
            return
        
        self.is_reconnecting = True
        self.reconnect_attempts = 0
        
        if self.on_reconnecting:
            self.on_reconnecting()
        
        threading.Thread(target=self._reconnect_loop, daemon=True).start()
    
    def _reconnect_loop(self):
        """Pokouší se znovu připojit."""
        print(f"🔄 Začínám reconnect (max {self.max_reconnect_attempts} pokusů)...")
        
        while self.is_reconnecting and self.reconnect_attempts < self.max_reconnect_attempts:
            self.reconnect_attempts += 1
            
            print(f"🔄 Pokus {self.reconnect_attempts}/{self.max_reconnect_attempts}...")
            
            if self.on_reconnecting:
                self.on_reconnecting(self.reconnect_attempts, self.max_reconnect_attempts)
            
            success = self.connect(
                self.server_ip,
                self.server_port,
                reconnect=True,
                auto_reconnect=True
            )
            
            if success:
                print("✅ Reconnect úspěšný!")
                self.is_reconnecting = False
                
                if self.on_reconnected:
                    self.on_reconnected()
                
                return
            
            time.sleep(self.reconnect_delay)
        
        # Vyčerpány pokusy
        print(f"❌ Reconnect selhal po {self.max_reconnect_attempts} pokusech")
        self.is_reconnecting = False
        
        if self.on_disconnect:
            self.on_disconnect()
    
    def _heartbeat_loop(self):
        """Pošle heartbeat každých 10s."""
        while self.running:
            while self.connected:
                time.sleep(10)
                self.send_message(MessageType.HEARTBEAT, [])
            time.sleep(3)
            
    def disconnect(self, stop_auto_reconnect: bool = True):
        """Bezpečné odpojení."""
        print("🔌 Manuální odpojení...")
        
        if stop_auto_reconnect:
            self.auto_reconnect = False
            self.is_reconnecting = False
        
        self.running = False
        self.connected = False
        
        if self.sock:
            try:
                self.send_message(MessageType.DISCONNECT, [])
            except:
                pass
    
    def stop_reconnect(self):
        """Zastaví reconnect."""
        self.is_reconnecting = False
        self.auto_reconnect = False
    
    def send_empty_trick(self):
        """Pošle prázdnou TRICK zprávu."""
        self.send_message(MessageType.TRICK, [])