# ============================================================
# ClientManager - Přidání connection timeout
# ============================================================

import socket
import threading
import time
from typing import List, Optional, Callable
from queue import Queue
from .Protocol import Protocol, MessageType

class ClientManager:
    def __init__(self):
        self.sock: Optional[socket.socket] = None
        
        # Connection state
        self.connected = False
        self.running = False
        self.disconnecting = False
        
        # Network options
        self.server_ip: Optional[str] = None
        self.server_port: Optional[int] = None
        
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
        
        # Synchronizace
        self.listen_ready = threading.Event()
        self.welcome_received = threading.Event()  # 🆕 Event pro WELCOME zprávu
        
        # Reconnect config
        self.auto_reconnect = False
        self.is_reconnecting = False
        self.reconnect_attempts = 0
        self.max_reconnect_attempts = 3
        self.reconnect_delay = 10
        self.last_pong = None
        
        # 🆕 Connection timeout
        self.connection_timeout = 10.0  # 10 sekund na spojení
        
        self.msgCounter = 0
        self.error_msg = ""
        
    # ============================================================
    # CONNECT - S TIMEOUTEM
    # ============================================================
    
    def connect(self, ip: str, port: int, reconnect: bool = False, 
                auto_reconnect: bool = True) -> bool:
        """Připojí se k serveru - S CONNECTION TIMEOUT."""
        try:
            # Uložíme server info
            self.server_ip = ip
            self.server_port = port
            self.auto_reconnect = auto_reconnect
            
            # Počkáme až starý listen thread skončí
            if self.listen_thread and self.listen_thread.is_alive():
                print("⏳ Čekám na ukončení starého listening threadu...")
                self.running = False
                self.listen_thread.join(timeout=2.0)
                if self.listen_thread.is_alive():
                    print("⚠️ Starý thread stále běží, pokračuji...")
            
            # Zavřeme staré připojení
            if self.sock:
                try:
                    self.sock.close()
                except:
                    pass
                self.sock = None
            
            # Vytvoříme nový socket
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.sock.settimeout(1.0)
            
            print(f"Připojuji se na {ip}:{port}...")
            self.sock.connect((ip, port))
            
            # Reset stavů
            self.connected = False
            self.is_reconnecting = False
            self.running = True
            self.disconnecting = False
            self.msgCounter = 0
            self.error_msg = ""
            
            # Reset events
            self.listen_ready.clear()
            self.welcome_received.clear()
            
            # Spustíme listening thread
            self.listen_thread = threading.Thread(target=self._listen_loop, daemon=True)
            self.listen_thread.start()
            
            # Heartbeat loop
            threading.Thread(target=self._heartbeat_loop, daemon=True).start()
            
            # Počkáme až listening thread naběhne
            print("⏳ Čekám na listening thread...")
            if not self.listen_ready.wait(timeout=5.0):
                print("❌ Listening thread se nespustil včas!")
                self.running = False
                if self.sock:
                    self.sock.close()
                return False
            print("✅ Listening thread připraven")
            
            # Spustíme message processing thread
            self.msg_processing_thread = threading.Thread(
                target=self._process_message_queue, daemon=True
            )
            self.msg_processing_thread.start()
            
            # Pošleme CONNECT nebo RECONNECT
            if self.nickname:
                if reconnect:
                    self.send_message(MessageType.RECONNECT, 
                                    [self.nickname, str(self.last_packet_id)])
                    print(f"🔄 Pokus o reconnect: {self.nickname}")
            
            # 🆕 ČEKÁME NA WELCOME/READY ZPRÁVU S TIMEOUTEM
            print(f"⏳ Čekám na odpověď od serveru (timeout: {self.connection_timeout}s)...")
            if not self.welcome_received.wait(timeout=self.connection_timeout):
                print("❌ Server neodpověděl včas (timeout)!")
                self.error_msg = "Server neodpověděl"
                self.disconnect(send_disconnect=False, msg="Connection timeout")
                return False
            
            print("✅ Server odpověděl, spojení aktivní")
            return True
            
        except socket.timeout:
            print(f"❌ Timeout při připojování k {ip}:{port}")
            self.error_msg = "Timeout při připojování"
            self.connected = False
            self.running = False
            if self.sock:
                try:
                    self.sock.close()
                except:
                    pass
                self.sock = None
            return False

        except Exception as e:
            print(f"❌ Chyba připojení: {e}")
            self.error_msg = str(e)
            self.connected = False
            self.running = False
            if self.sock:
                try:
                    self.sock.close()
                except:
                    pass
                self.sock = None
            return False
    
    # ============================================================
    # HANDLE MESSAGE - Se signalizací WELCOME
    # ============================================================
    
    def _handle_message(self, data: str):
        try:
            packet_id, _, msg_type, fields = Protocol.deserialize(data)

            self.last_packet_id = packet_id

            print(f"📨 Přijatá zpráva: {msg_type.name}")
            if fields:
                print(f"   Data: {fields}")
            
            if msg_type == MessageType.PONG:
                self.last_pong = time.time()
                return
            
            if msg_type in (MessageType.WELCOME, MessageType.AUTHORIZE):
                print("✅ Přijato potvrzení od serveru")
                self.welcome_received.set()  # 🆕 Signalizuj úspěch
                self.connected = True
            
            # RECONNECT také signalizuje úspěch
            elif msg_type == MessageType.RECONNECT:
                print("✅ Reconnect potvrzen serverem")
                self.welcome_received.set()  # 🆕 Signalizuj úspěch
                self.connected = True
                self.last_pong = time.time()
                
            # DISCONNECT zpracujeme speciálně
            elif msg_type == MessageType.DISCONNECT:
                print("⚠️ Server poslal DISCONNECT")
                self.auto_reconnect = False
                
                if self.on_disconnect:
                    if fields:
                        self.disconnect(msg=fields[0], send_disconnect=False)
                    else:
                        self.disconnect(send_disconnect=False)
                return

            if self.on_message:
                self.on_message(msg_type, fields)

            self.msgCounter = 0

        except Exception as e:
            print(f"❌ Chyba parsování zprávy: {e}")
            self.msgCounter += 1
            if self.msgCounter >= 3:
                self.disconnect(msg="Server posílá nesprávné zprávy!")
    
    # ============================================================
    # DISCONNECT - Vylepšený
    # ============================================================
    
    def disconnect(self, stop_auto_reconnect: bool = True, 
                   send_disconnect: bool = True, msg: str = ""):
        """Bezpečné odpojení."""
        if self.disconnecting:
            print("🔄 Disconnect už probíhá, přeskakuji...")
            return
            
        self.disconnecting = True
        print("🔌 Odpojuji od serveru...")
        
        if stop_auto_reconnect:
            self.auto_reconnect = False
            self.is_reconnecting = False
        
        # Nejdřív zastavíme listening
        self.running = False
        self.connected = False
        
        # 🆕 Signalizuj welcome_received aby connect() nečekal
        self.welcome_received.set()
        
        # Pošleme DISCONNECT PŘED zavřením socketu
        if send_disconnect and self.sock:
            try:
                print("📤 Posílám DISCONNECT...")
                self.send_message(MessageType.DISCONNECT, [])
                time.sleep(0.1)
            except Exception as e:
                print(f"⚠️ Chyba při posílání DISCONNECT: {e}")
        
        # Počkáme až listening thread skončí
        if self.listen_thread and self.listen_thread.is_alive():
            print("⏳ Čekám na ukončení listening threadu...")
            self.listen_thread.join(timeout=2.0)
            if self.listen_thread.is_alive():
                print("⚠️ Listening thread se neukončil včas")
        
        # Zavřeme socket
        if self.sock:
            try:
                self.sock.close()
            except Exception as e:
                print(f"⚠️ Chyba při zavírání socketu: {e}")
            self.sock = None
        
        # Callback NAKONEC
        if self.on_disconnect:
            if msg:
                self.on_disconnect([msg])
            elif self.error_msg:
                self.on_disconnect([self.error_msg])
            else:
                self.on_disconnect([""])
        
        self.disconnecting = False
        print("✅ Disconnect dokončen")
    
    def _start_reconnect(self):
        """Spustí reconnect thread."""
        if self.is_reconnecting:
            print("🔄 Reconnect už běží, přeskakuji...")
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
            
            print(f"⏳ Čekám {self.reconnect_delay}s před dalším pokusem...")
            time.sleep(self.reconnect_delay)
        
        print(f"❌ Reconnect selhal po {self.max_reconnect_attempts} pokusech")
        self.is_reconnecting = False
        self.auto_reconnect = False
        
        if self.on_disconnect:
            self.on_disconnect(["Reconnect selhal"])
    
    def send_message(self, msg_type: MessageType, fields: List[str]) -> bool:
        """Pošle zprávu serveru."""
        try:
            if not self.sock:
                print("❌ Socket není inicializován")
                return False
            
            client_number = self.number if self.number is not None else 0
            data = Protocol.serialize(self.last_packet_id, client_number, msg_type, fields)
            
            print(f"📤 Odesílám: {msg_type.name} ({len(data)} bytů)")
            self._send_exactly(data)
            
            return True
            
        except Exception as e:
            print(f"❌ Chyba odeslání: {e}")
            self.connected = False
            return False
    
    def _send_exactly(self, data: bytes):
        """Pošle všechna data."""
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
    
    def _listen_loop(self):
        """Listening thread."""
        print("🔄 Listening thread spuštěn")
        self.listen_ready.set()
        
        buffer = b""

        while self.running:
            try:
                chunk = self.sock.recv(256)
                if not chunk:
                    print("🔌 Server uzavřel spojení")
                    break
                    
                buffer += chunk

                while b'\n' in buffer:
                    line, buffer = buffer.split(b'\n', 1)
                    line += b'\n'

                    msg_str = line.decode("utf-8", errors="replace")

                    if not Protocol.is_valid_message_string(msg_str):
                        print("❌ Neplatná zpráva (string validation)")
                        self.msgCounter += 1
                        if self.msgCounter >= 3:
                            self.error_msg = "Server posílá neplatná data!"
                            self.running = False
                            break
                        continue

                    self.msg_queue.put(msg_str)
                    
            except socket.timeout:
                continue
            except Exception as e:
                print(f"❌ Listening chyba: {e}")
                break
        
        print("🔌 Listening thread končí")
        
        if self.running:
            print("🔌 Listening loop skončil neočekávaně")
            self._handle_connection_lost()
        else:
            print("🔌 Listening loop skončil očekávaně")
    
    def _process_message_queue(self):
        """Zpracovává zprávy z fronty."""
        print("🔄 Message processing thread spuštěn")
        
        while self.running:
            try:
                data = self.msg_queue.get(timeout=0.1)
                self._handle_message(data)
                self.msg_queue.task_done()
            except:
                continue
        
        print("🛑 Message processing thread ukončen")
    
    def _handle_connection_lost(self):
        """Zpracuje ztrátu spojení."""
        if self.disconnecting:
            print("🔄 Disconnect už probíhá, přeskakuji...")
            return
        
        if not self.running:
            print("🔄 running == False, disconnect byl zavolán, přeskakuji...")
            return
            
        print("🔌 Spojení ztraceno!")
        
        was_connected = self.connected
        self.connected = False
        self.running = False
        
        print(f"⏳ Čekám na zpracování zbývajících zpráv ({self.msg_queue.qsize()})...")
        try:
            self.msg_queue.join()
        except:
            pass
        
        time.sleep(0.1)
        
        if self.disconnecting:
            print("🔄 Mezitím proběhl disconnect, přeskakuji auto-reconnect")
            return
        
        if not self.auto_reconnect:
            print("🚫 Auto-reconnect zakázán")
            if self.on_disconnect:
                self.on_disconnect(["Odpojen od serveru"])
            return
        
        if self.nickname and was_connected:
            print("🔄 Spouštím auto-reconnect...")
            self._start_reconnect()
        else:
            reasons = []
            if not self.nickname:
                reasons.append("není nickname")
            if not was_connected:
                reasons.append("nebyl connected")
            
            print(f"🚫 Auto-reconnect neproveden: {', '.join(reasons)}")
            self.disconnect(send_disconnect=False, msg=self.error_msg)
    
    def stop_reconnect(self):
        """Zastaví probíhající reconnect."""
        print("🛑 Zastavuji reconnect...")
        self.is_reconnecting = False
        self.auto_reconnect = False
        
    def _heartbeat_loop(self):
        while self.running:
            time.sleep(3)

            if not self.connected or self.disconnecting:
                continue

            # 1Pošli PING
            ok = self.send_message(MessageType.PING, [])
            if not ok:
                print("💀 Nepodařilo se poslat PING")
                self._handle_connection_lost()
                return

            # Kontrola PONG
            if self.last_pong is not None:
                if time.time() - self.last_pong > 12:
                    print("💀 Server neodpovídá (PONG timeout)")
                    self._handle_connection_lost()
                    return
    
    def send_empty_trick(self):
        """Pošle prázdnou TRICK zprávu."""
        self.send_message(MessageType.TRICK, [])