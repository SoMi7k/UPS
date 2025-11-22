import pygame
import sys
import time
from enum import Enum
import threading
from src.gui.GuiManager import GuiManager
from src.ClientManager import ClientManager, MessageType
from src.GameManager import GameManager
from src.game.game import Game

class GameState(Enum):
    LOBBY = 0           # Zadávání IP/Port/Nickname
    CONNECTING = 1      # Připojování k serveru
    WAITING = 2         # Čekání na ostatní hráče (WAIT_LOBBY)
    PLAYING = 3         # Hra běží

class App:
    def __init__(self):
        # === UI FUNKCE + VYKRESLENÍ OSTATNÍCH ČÁSTÍ HRY ===
        self.guiManager = GuiManager()
        
        # === CLIENT/SPOJENÍ ===
        self.client = ClientManager()
        self.setup_client_callbacks()
        
        # === POČET HRÁČŮ ===
        self.required_players = 0    # Potřebný počet hráčů
        self.connected_players = 0
        
        # === VYKRESLENÍ HRY MARIÁŠ ===
        self.gameManager = None
        
        # === OSTATNÍ ===
        self.clock = pygame.time.Clock()
        self.state = GameState.LOBBY
        self.lock = threading.Lock()
        self.invalid = None
        
    # ============================================================
    # 
    # ============================================================
            
    def set_state(self, new_state):
        with self.lock:
            self.state = new_state

    def get_state(self):
        with self.lock:
            return self.state

    # ============================================================
    # SETUP CLIENT CALLBACKS - Nastavení callbacků od serveru
    # ============================================================
    
    def setup_client_callbacks(self):
        """Nastaví callbacky pro zprávy od serveru."""
        print("🔧 Nastavuji client callbacks...")
        
        # Callback pro přijaté zprávy
        self.client.on_message = self.handle_server_message
        
        # Callback pro odpojení
        self.client.on_disconnect = self.handle_disconnect
    
    def handle_server_message(self, msg_type: MessageType, data: dict):
        """
        Hlavní handler pro VŠECHNY zprávy od serveru.
        Volá se z listening threadu klienta!
        """
        print(f"📨 Přijata zpráva: {msg_type.value}")
        print(f"   Data: {data}")
        
        # ===== WELCOME - První zpráva po připojení =====
        if msg_type == MessageType.WELCOME:
            self.handle_welcome(data)
        
        # ===== WAIT_LOBBY - Čekání na další hráče =====
        elif msg_type == MessageType.WAIT_LOBBY:
            self.handle_wait_lobby(data)
        
        # ===== GAME_START - Hra začíná =====
        elif msg_type == MessageType.GAME_START:
            self.handle_game_start(data)
        
        # ===== GAME_START - Hra začíná =====
        elif msg_type == MessageType.CLIENT_DATA:
            self.handle_client_data(data)
        
        # ===== STATE - Aktualizace stavu hry =====
        elif msg_type == MessageType.STATE:
            self.handle_game_state(data)
        
        # ===== YOUR_TURN - Je můj tah =====
        elif msg_type == MessageType.YOUR_TURN:
            self.handle_your_turn(data)
        
        # ===== ERROR - Chybová zpráva =====
        elif msg_type == MessageType.ERROR:
            self.handle_error(data)
            
        elif msg_type == MessageType.INVALID:
            self.handle_invalid(data)
            
        elif msg_type == MessageType.RESULT:
            self.handle_result(data)
    
    # ============================================================
    # HANDLERS PRO JEDNOTLIVÉ TYPY ZPRÁV
    # ============================================================
    
    def handle_welcome(self, data: dict):
        """Zpracuje WELCOME zprávu od serveru."""
        print("👋 Zpracovávám WELCOME...")
        
        self.client.number = int(data["playerNumber"])
        self.client.session_id = data["sessionId"]
        self.lobby_id = int(data["lobby"])
        self.required_players = data.get("requiredPlayers", 2)
        self.client.nickname = self.guiManager.nickname_input.text
        
        print(f"✅ Moje session ID: #{self.client.session_id}\n \
                ✅ Připojeno do lobby {self.lobby_id}\n \
                ✅ Hra Mariáš pro {self.required_players}")
        
        self.gameManager = GameManager(self.required_players, self.client, self.guiManager)
        self.set_state(GameState.CONNECTING)
        
        self.client.send_message(MessageType.CONNECT, {"nickname": self.client.nickname})
        print(f"📤 Posílám nickname: {self.client.nickname}")
    
    def handle_wait_lobby(self, data: dict):
        """Zpracuje WAIT_LOBBY zprávu."""
        print("⏳ Zpracovávám WAIT_LOBBY...")
        
        self.connected_players = data.get("current", 0)
        
        actual_gameState = self.get_state()
        if actual_gameState == GameState.PLAYING:
            self.gameManager.game = Game(self.required_players, self.client.number)
            
        self.set_state(GameState.WAITING)
        
        print(f"⏳ Čekám na hráče: {self.connected_players}/{self.required_players}")
    
    def handle_game_start(self, data: dict):
        """Zpracuje GAME_START zprávu."""
        print("🎮 Zpracovávám GAME_START...")
        print("🎮 HRA ZAČÍNÁ!")
    
        if not data:
            self.set_state(GameState.CONNECTING)
            
        self.gameManager.game_start_reader(data)
        
        # Přepnout do herního stavu
        self.set_state(GameState.PLAYING)
        
        print("✅ Přepínám do PLAYING stavu")
    
    def handle_game_state(self, data: dict):
        """Zpracuje STATE zprávu - aktualizace stavu hry."""
        print("🔄 Zpracovávám STATE...")
        self.invalid = None
        self.gameManager.state_reader(data)
    
    def handle_your_turn(self, data: dict):
        """Zpracuje YOUR_TURN zprávu - je můj tah."""
        print("🔔 Je můj tah!")
        self.gameManager.game.active_player = True
        
    def handle_result(self, data: dict):
        """Zpracuje RESULT zprávu od serveru."""
        print("🔔 Přišla zpráva o výsledku!")
        self.gameManager.game_result_reader(data)
        
    def handle_error(self, data: dict):
        """Zpracuje ERROR zprávu od serveru."""
        error_msg = data["message"]
        print(f"❌ CHYBA OD SERVERU: {error_msg}")
        msg = "ERROR: " + error_msg + "\n\n" + "Přepojuji do Lobby..."
        self.gameManager.show_error_messages(msg)
        time.sleep(2)
        
        # Odpojit a vrátit do lobby
        self.client.disconnect()
        self.set_state(GameState.LOBBY)
        
    def handle_client_data(self, data: dict):
        self.gameManager.player_reader(data["client"])
        
    def handle_invalid(self, data: dict):
        self.invalid = data["data"]
    
    def handle_disconnect(self):
        """Callback při odpojení od serveru."""
        print("🔌 Odpojen od serveru!")
        self.set_state(GameState.LOBBY)
    
    # ============================================================
    # AKCE OD UŽIVATELE - Připojení k serveru
    # ============================================================
    
    def connect_to_server(self):
        """Připojí se k serveru s údaji z lobby."""
        ip = self.guiManager.ip_input.text
        port_str = self.guiManager.port_input.text
        
        try:
            port = int(port_str)
        except ValueError:
            print("❌ Neplatný port!")
            return False
        
        print(f"🔌 Připojuji se na {ip}:{port}...")
        
        self.set_state(GameState.CONNECTING)
        
        success = self.client.connect(ip, port)
        
        if not success:
            print("❌ Připojení selhalo!")
            self.set_state(GameState.LOBBY)
            return False
        
        print("✅ Připojen!")
        return True
        
    # ============================================================
    # EVENT HANDLING
    # ============================================================
    
    def handle_lobby_event(self, event):
        """Zpracuje události v lobby."""
        if self.guiManager.ip_input.handle_event(event):
            return "connect"
        if self.guiManager.port_input.handle_event(event):
            return "connect"
        if self.guiManager.nickname_input.handle_event(event):
            return "connect"
        
        if self.guiManager.connect_button.is_clicked(event):
            return "connect"
        if self.guiManager.quit_button.is_clicked(event):
            pygame.quit()
            sys.exit()
        
        return None
    
    def handle_waiting_event(self, event):
        if self.guiManager.quit_button.is_clicked(event):
            pygame.quit()
            sys.exit()
        
    def handle_playing_event(self, event):
        """Zpracuje herní události (klikání na karty nebo tlačítka)."""
        if event.type == pygame.MOUSEBUTTONDOWN and self.gameManager.game.active_player:
            for rect, label in self.gameManager.active_rects:
                if rect.collidepoint(event.pos):
                    print(f"🎯 Kliknuto na: {label}")
                    
                    # Rozlišíme, jestli jde o kartu nebo volbu
                    if any(ch.isdigit() or ch in "♥♦♣♠" for ch in label):
                        # 🃏 karta
                        self.client.send_message(MessageType.CARD, {"card": label})
                        print(f"📤 Odesílám kartu: {label}")
                    else:
                        # 🔘 tlačítko volby (např. "BETL", "DURCH", "Špatný", "ANO")
                        if label == "ANO" or label == "NE":
                            self.client.send_message(MessageType.RESET, {"label": label})
                            print(f"📤 Odesílám volbu: {label}")
                        else:                        
                            self.client.send_message(MessageType.BIDDING, {"label": label})
                            print(f"📤 Odesílám volbu: {label}")
                    
                    # po kliknutí hráč už nehraje
                    self.gameManager.game.active_player = False
                    break
    
    # ============================================================
    # HLAVNÍ SMYČKA
    # ============================================================
    
    def run(self):
        """Hlavní smyčka aplikace."""
        running = True
        
        while running:
            for event in pygame.event.get():
                if event.type == pygame.QUIT:
                    running = False
                
                # Zpracování událostí podle stavu
                if self.get_state() == GameState.LOBBY:
                    action = self.handle_lobby_event(event)
                    if action == "connect":
                        self.connect_to_server()
                        
                elif self.get_state() == GameState.WAITING:
                    action = self.handle_waiting_event(event)
                    if action == "dissconect":
                        self.client.disconnect()
                
                elif self.get_state() == GameState.PLAYING:
                    self.handle_playing_event(event)
            
            # Vykreslování podle stavu
            if self.get_state()== GameState.LOBBY:
                self.guiManager.draw_lobby()
            elif self.get_state() == GameState.WAITING:
                self.guiManager.draw_waiting(self.connected_players, self.required_players, self.client.number)
            elif self.get_state() == GameState.CONNECTING:
                self.guiManager.draw_connecting()
            elif self.get_state() == GameState.PLAYING:
                self.gameManager.draw_playing()
            
            pygame.display.flip()
            self.clock.tick(60)
        
        # Cleanup
        if self.client.connected:
            self.client.disconnect()
        
        pygame.quit()
        sys.exit()