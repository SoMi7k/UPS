import pygame
import sys
import time
from enum import Enum
import threading
from .View.GuiManager import GuiManager
from .Client.ClientManager import ClientManager, MessageType
from .GameManager import GameManager
from .Game.Game import Game
from .View.Validator import InputValidator

class GameState(Enum):
    LOBBY = 0           # Zadávání IP/Port/Nickname
    CONNECTING = 1      # Připojování k serveru
    WAITING = 2         # Čekání na připojení ostatních hráčů v herní místnosti
    DISCONNECTION = 3   # Čekání na odpojeného hráče
    PLAYING = 4         # Hra běží
    RECONNECTING = 5    # Znovupřipojování
    HELP = 6            # Nápověda

class Gui:
    def __init__(self):
        # === UI FUNKCE + VYKRESLENÍ OSTATNÍCH ČÁSTÍ HRY ===
        self.guiManager = GuiManager()
        
        # === CLIENT/SPOJENÍ ===
        self.client = ClientManager(self.guiManager)
        self.setup_client_callbacks()
        
        # === POČET HRÁČŮ ===
        self.required_players = 0    # Potřebný počet hráčů
        self.connected_players = 0
        
        # 🆕 RECONNECT INFO
        self.reconnect_attempt = 0
        self.reconnect_max = 60
        
        # === VYKRESLENÍ HRY MARIÁŠ ===
        self.gameManager = None
        
        # === OSTATNÍ ===
        self.clock = pygame.time.Clock()
        self.state = GameState.LOBBY
        self.lock = threading.Lock()
        self.waiting_text = None
        
        # === Optimalizační flagy ===
        self.needs_redraw = True
        self.last_mouse_pos = None
        self.fps_limit = 30
        
    # ============================================================
    # Inteligentní překreslování
    # ============================================================
    
    def mark_for_redraw(self):
        """Označí obrazovku pro překreslení."""
        self.needs_redraw = True
    
    def should_redraw(self) -> bool:
        """Rozhodne, zda je potřeba překreslit."""
        # Vždy překresli pokud je flag nastaven
        if self.needs_redraw:
            return True
        
        # Překresli pokud se hýbe myš (pro hover efekty)
        current_mouse_pos = pygame.mouse.get_pos()
        if current_mouse_pos != self.last_mouse_pos:
            self.last_mouse_pos = current_mouse_pos
            return True
        
        # Animované stavy potřebují neustálé překreslování
        if self.get_state() in [GameState.CONNECTING, GameState.RECONNECTING]:
            return True
        
        return False
    
        
    # ============================================================
    # GameState mutex
    # ============================================================
            
    def set_state(self, new_state: GameState):
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
        
        # 🆕 Callback pro reconnecting
        self.client.on_reconnecting = self.handle_reconnecting
        
        # 🆕 Callback pro úspěšný reconnect
        self.client.on_reconnected = self.handle_reconnected
    
    def handle_server_message(self, msg_type: MessageType, data: list):
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
        
        # ===== CLIENT_DATA - Data o hráči =====
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
        
        # ===== INVALID - Nesprávný krok klienta =====
        elif msg_type == MessageType.INVALID:
            self.handle_invalid(data)
        
        # ===== RESULT - Výsledek hry =====
        elif msg_type == MessageType.RESULT:
            self.handle_result(data)
        
        # ===== STATUS - Odpojení klintů =====
        elif msg_type == MessageType.STATUS:
            self.handle_status(data)
            
    # ============================================================
    # HANDLERS PRO JEDNOTLIVÉ TYPY ZPRÁV
    # ============================================================
    
    def handle_welcome(self, data: list):
        """Zpracuje WELCOME zprávu od serveru."""
        print("👋 Zpracovávám WELCOME...")
        
        if len(data) != 3:
            print("Chybný počet argumentů: Welcome")
            self.client.disconnect()
            return
        
        self.client.number = int(data[0])
        self.lobby_id = int(data[1])
        self.required_players = int(data[2])
        self.client.nickname = self.guiManager.nickname_input.text
        
        print(f"✅ Připojeno do lobby {self.lobby_id}")
        print(f"✅ Hra Mariáš pro {self.required_players}")
        
        self.gameManager = GameManager(self.required_players, self.client, self.guiManager)
        self.set_state(GameState.CONNECTING)
        
        self.client.send_message(MessageType.CONNECT, [self.client.nickname])
        print(f"📤 Posílám nickname: {self.client.nickname}")
    
    def handle_wait_lobby(self, data: list):
        """Zpracuje WAIT_LOBBY zprávu."""
        print("⏳ Zpracovávám WAIT_LOBBY...")
        self.connected_players = data[0]
        actual_gameState = self.get_state()
        self.set_state(GameState.WAITING)
        
        if actual_gameState == GameState.PLAYING:
            self.gameManager.game = Game(self.required_players, self.client.number)
            
        print(f"⏳ Čekám na hráče: {self.connected_players}/{self.required_players}")
    
    def handle_game_start(self, data: list):
        """Zpracuje GAME_START zprávu."""
        print("🎮 Zpracovávám GAME_START...")
        print("🎮 HRA ZAČÍNÁ!")
        
        self.gameManager.set_game()
    
        if not data:
            self.set_state(GameState.CONNECTING)
            
        self.gameManager.game_start_reader(data)
        
        print("🎮 GameStart Přečtený!")
        
        # Přepnout do herního stavu
        self.set_state(GameState.PLAYING)
        
        print("✅ Přepínám do PLAYING stavu")
    
    def handle_game_state(self, data: list):
        """Zpracuje STATE zprávu - aktualizace stavu hry."""
        print("🔄 Zpracovávám STATE...")
        self.gameManager.invalid = None
        self.gameManager.state_reader(data)
        
        print("🎮 GameState Přečtený!")
    
    def handle_your_turn(self, data: list):
        """Zpracuje YOUR_TURN zprávu - je můj tah."""
        print("🔔 Je můj tah!")
        self.gameManager.game.active_player = True
        
    def handle_result(self, data: list):
        """Zpracuje RESULT zprávu od serveru."""
        print("🔔 Přišla zpráva o výsledku!")
        self.gameManager.game_result_reader(data)
        
    def handle_error(self, data: list):
        """Zpracuje ERROR zprávu od serveru."""
        error_msg = data[0]
        print(f"❌ CHYBA OD SERVERU: {error_msg}")
        msg = "ERROR: " + error_msg + "\n\n" + "Přepojuji do Lobby..."
        self.gameManager.show_error_messages(msg)
        time.sleep(2)
        
        # Odpojit a vrátit do lobby
        self.client.disconnect()
        self.set_state(GameState.LOBBY)
        
    def handle_client_data(self, data: list):
        """Zpracuje CLIENT_DATA zprávu."""
        self.gameManager.player_reader(data)
        
    def handle_invalid(self, data: list):
        """Zpracuje INVALID zprávu."""
        self.gameManager.invalid = data[0]
    
    def handle_disconnect(self):
        """Callback při odpojení od serveru."""
        print("🚫 Odpojen od serveru - zakazuji auto-reconnect")
        self.auto_reconnect = False
        self.is_reconnecting = False
        self.set_state(GameState.LOBBY)
    
    def handle_reconnecting(self, attempt: int|None = None, max_attempts: int|None = None):
        """Callback volaný při pokusu o reconnect."""
        print(f"🔄 Reconnecting... ({attempt}/{max_attempts})")
        
        if attempt:
            self.reconnect_attempt = attempt
            self.reconnect_max = max_attempts
        
        # Přepneme do RECONNECTING stavu pro zobrazení UI
        if self.get_state() != GameState.RECONNECTING:
            self.set_state(GameState.RECONNECTING)
    
    def handle_reconnected(self):
        """Callback volaný při úspěšném reconnectu."""
        print("✅ Úspěšně reconnectnut!")
        
        # Vrátíme se do PLAYING stavu
        self.set_state(GameState.PLAYING)
    
    def handle_status(self, data: list):
        # 1 - disconnect | 2 - reconnecting | 3 - reconnected
        code = int(data[0])
        nickname = data[1]
        
        match code:
            case 1:
                print("🔔 Vypisuji STATUS 1")
                #self.gameManager.invalid = f"Hráč {nickname} se odpojil. HRA KONČÍ! Přesouvám do lobby..."
                self.client.disconnect()
                self.guiManager.error_message = f"Hráč {nickname} se odpojil. HRA UKONČENA! Kontumačně vyhráváte."
                self.set_state(GameState.LOBBY)
            case 2:
                print("🔔 Vypisuji STATUS 2")
                reconnect_timeout = data[2]
                self.guiManager.waiting_message = f"Hráč {nickname} se odpojil. Má {reconnect_timeout}s na připojení."
                self.set_state(GameState.WAITING)
            case 3:
                print("🔔 Vypisuji STATUS 3")
                self.guiManager.waiting_message = ""
                self.set_state(GameState.PLAYING)
            case _:
                print("🔔 Ani jeden status nebyl zavolán")
        
    # ============================================================
    # AKCE OD UŽIVATELE - Připojení k serveru
    # ============================================================
    
    def connect_to_server(self, reconnect: bool):
        """Připojí se k serveru s údaji z lobby - s validací."""
        ip = self.guiManager.ip_input.text
        port_str = self.guiManager.port_input.text
        nickname = self.guiManager.nickname_input.text
        
        valid, error_msg, port = InputValidator.validate_all(ip, port_str, nickname)
        
        if not valid:
            print(f"❌ Validační chyba: {error_msg}")
            self.guiManager.error_message = error_msg
            return False
        
        # Vyčistit chybovou zprávu
        self.guiManager.error_message = ""
        
        print(f"🔌 Připojuji se na {ip}:{port} jako '{nickname}'...")
        
        self.set_state(GameState.CONNECTING)
        
        success = self.client.connect(ip, port, reconnect, True)
        
        if not success:
            print("❌ Připojení selhalo!")
            self.guiManager.error_message = "Připojení k serveru selhalo!"
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
        
        if self.guiManager.recconect_button.is_clicked(event):
            return "reconnect"
        
        if self.guiManager.quit_button.is_clicked(event):
            pygame.quit()
            sys.exit()
            
        if self.guiManager.help_button.is_clicked(event):
            print("ℹ️ Otevírám help screen")
            self.state = GameState.HELP
            return
        
        return None
    
    def handle_help_events(self, event):
        if self.guiManager.help_back_button.is_clicked(event):
            self.set_state(GameState.LOBBY)
    
    def handle_waiting_event(self, event):
        """Zpracuje události při čekání na hráče."""
        if self.guiManager.back_button.is_clicked(event):
            self.client.disconnect()
            self.set_state(GameState.LOBBY)
    
    def handle_reconnecting_event(self, event):
        """Zpracuje události při reconnectingu."""
        if self.guiManager.back_button.is_clicked(event):
            # Zastavíme reconnect a vrátíme se do lobby
            self.client.stop_reconnect()
            self.client.disconnect()
            self.set_state(GameState.LOBBY)
    
    def handle_playing_event(self, event):
        """Zpracuje herní události (klikání na karty nebo tlačítka)."""

        if event.type == pygame.MOUSEBUTTONUP and event.button == 1:
            self.gameManager.click_lock = False
            return

        if event.type != pygame.MOUSEBUTTONDOWN or event.button != 1:
            return

        if self.gameManager.click_lock:
            return

        if not self.gameManager.game.active_player:
            return

        if not self.gameManager.active_rects:
            return

        for rect, label in self.gameManager.active_rects:
            if rect.collidepoint(event.pos):
                self.gameManager.click_lock = True
                print(f"🎯 Kliknuto na: {label}")

                # Rozlišení typu akce
                if any(ch.isdigit() or ch in "♥♦♣♠" for ch in label):
                    self.client.send_message(MessageType.CARD, [label])
                    print(f"📤 Odesílám kartu: {label}")
                else:
                    if label in ("ANO", "NE"):
                        self.client.send_message(MessageType.RESET, [label])
                    else:
                        self.client.send_message(MessageType.BIDDING, [label])
                    print(f"📤 Odesílám volbu: {label}")

                self.gameManager.game.active_player = False
                break
    
    # ============================================================
    # HLAVNÍ SMYČKA
    # ============================================================
    
    def run(self):
        """Hlavní smyčka aplikace - OPTIMALIZOVANÁ."""
        running = True
        
        while running:
            # === EVENT HANDLING ===
            events = pygame.event.get()
            
            for event in events:
                if event.type == pygame.QUIT:
                    self.client.disconnect(stop_auto_reconnect=True)
                    running = False
                    continue
                
                # Při jakémkoli eventu vynutit překreslení
                self.mark_for_redraw()
                    
                # Zpracování událostí podle stavu
                if self.get_state() == GameState.LOBBY:
                    action = self.handle_lobby_event(event)
                    if action == "connect":
                        self.connect_to_server(False)
                    elif action == "reconnect":
                        if self.client.nickname:
                            self.connect_to_server(True)
                        else:
                            self.guiManager.error_message = "Reconnect nelze provést!"
                            
                elif self.get_state() == GameState.HELP:
                    self.handle_help_events(event)
                        
                elif self.get_state() == GameState.WAITING:
                    self.handle_waiting_event(event)
                
                elif self.get_state() == GameState.RECONNECTING:
                    self.handle_reconnecting_event(event)
                
                elif self.get_state() == GameState.PLAYING:
                    self.handle_playing_event(event)
            
            # === RENDERING (pouze když je potřeba) ===
            if self.should_redraw():
                if self.get_state() == GameState.LOBBY:
                    self.guiManager.draw_lobby()
                elif self.get_state() == GameState.HELP:
                    self.guiManager.draw_help()
                elif self.get_state() == GameState.WAITING:
                    self.guiManager.draw_waiting(self.connected_players, self.required_players, self.client.number)
                elif self.get_state() == GameState.CONNECTING:
                    self.guiManager.draw_connecting()
                elif self.get_state() == GameState.RECONNECTING:
                    self.guiManager.draw_reconnecting(self.reconnect_attempt, self.reconnect_max)
                elif self.get_state() == GameState.PLAYING:
                    self.gameManager.draw_playing()
                
                pygame.display.flip()
                self.needs_redraw = False  # Resetuj flag
            
            # 🆕 Snížená frekvence z 60 na 30 FPS
            self.clock.tick(self.fps_limit)
        
        # Cleanup
        if self.client.connected:
            self.client.disconnect()
        
        pygame.quit()
        sys.exit()