import pygame
import sys
from enum import Enum
from button import Button
from inputbox import InputBox
from client import Client, MessageType
from msg_handler import msgHandler
from game import Game
from card import Card, State, Mode
import time

# Barvy
WHITE = (255, 255, 255)
BLACK = (0, 0, 0)
GRAY = (200, 200, 200)
DARK_GRAY = (100, 100, 100)
LIGHT_GRAY = (240, 240, 240)
GREEN = (0, 255, 0)
YELLOW = (200, 200, 0)
DARK_YELLOW = (175, 175, 20)
DARK_GREEN = (0, 100, 0)
RED = (220, 20, 60)
BLUE = (70, 130, 180)
GOLD = (255, 215, 0)

# Konstanty
WIDTH = 1200
HEIGHT = 700
CARD_WIDTH, CARD_HEIGHT = 70, 100
IMG_DIR = "images\\wooden_table.jpg"

class GameState(Enum):
    LOBBY = 0           # Zadávání IP/Port/Nickname
    CONNECTING = 1      # Připojování k serveru
    WAITING = 2         # Čekání na ostatní hráče (WAIT_LOBBY)
    PLAYING = 3         # Hra běží

class GUI:
    def __init__(self):
        pygame.init()
        self.screen = pygame.display.set_mode((WIDTH, HEIGHT))
        pygame.display.set_caption("Mariáš - Multiplayer")
        
        # Fonty
        self.title_font = pygame.font.Font(None, 72)
        self.font_large = pygame.font.Font(None, 48)
        self.font_medium = pygame.font.Font(None, 36)
        self.font_small = pygame.font.Font(None, 28)
        self.font = pygame.font.SysFont("Arial", 28, bold=True)
        
        self.clock = pygame.time.Clock()
        self.state = GameState.LOBBY
        self.offsets = {}  # Pro animaci karet
        
        # === STAV APLIKACE ===
        self.player_number = None       # Číslo hráče (0-2) z WELCOME
        self.nickname = None            # Jméno hráče (Player) z WELCOME
        self.waiting_message = ""       # Text pro WAITING stav
        self.connected_players = 0      # Počet připojených hráčů
        self.required_players = None    # Potřebný počet hráčů
        self.invalid = None
        self.waiting_for_trick = False
        
        # === HERNÍ DATA ===
        self.game = None                # Instance Game (dostaneme od serveru)
        self.played_cards = []          # Zahrané karty na stole
        self.active_rects = []          # Klikatelné oblasti
        
        # Lobby komponenty
        self.setup_lobby()
        
        # Client (spojení)
        self.client = Client()
        self.setup_client_callbacks()
        
        # Pozadí
        self.background = self.create_background()
        
        # Message handler
        self.game = None
        self.msg_handler = msgHandler()
    
    def create_background(self):
        """Vytvoří gradient pozadí nebo načte obrázek."""
        try:
            # Zkusíme načíst obrázek
            image = pygame.image.load(IMG_DIR).convert_alpha()
            return pygame.transform.scale(image, (WIDTH, HEIGHT))
        except:
            # Fallback na gradient
            print("⚠ Nepodařilo se načíst pozadí, použiji gradient")
            background = pygame.Surface((WIDTH, HEIGHT))
            for y in range(HEIGHT):
                color_value = int(20 + (y / HEIGHT) * 40)
                pygame.draw.line(background, (color_value, color_value + 20, color_value + 10), 
                               (0, y), (WIDTH, y))
            return background
    
    def setup_lobby(self):
        """Nastaví komponenty lobby."""
        center_x = WIDTH // 2
        
        # Input boxy
        self.ip_input = InputBox(center_x - 150, 250, 300, 50, "Server IP:", "127.0.0.1")
        self.port_input = InputBox(center_x - 150, 350, 300, 50, "Port:", "10000")
        self.nickname_input = InputBox(center_x - 150, 450, 300, 50, "Nickname:", "Player")
        
        # Tlačítka
        self.connect_button = Button(center_x - 100, 550, 200, 60, "Připojit", YELLOW, DARK_YELLOW)
        self.quit_button = Button(center_x - 100, 620, 200, 60, "Ukončit", RED, (180, 0, 0))
    
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
        
        # TODO - Dodělat reconnect
        self.player_number = int(data["playerNumber"])
        self.session_id = data["sessionId"]
        self.lobby_id = int(data["lobby"])
        self.required_players = int(data["requiredPlayers"])
        print(f"✅ Moje session ID: #{self.session_id}\n \
                ✅ Připojeno do lobby {self.lobby_id}\n \
                ✅ Hra Mariáš pro {self.required_players}")
        
        self.game = Game(self.required_players, self.player_number)
        self.msg_handler.set_game(self.game)
        
        self.state = GameState.CONNECTING
        
        self.nickname = self.nickname_input.text
        self.client.send_message(MessageType.CONNECT, {"nickname": self.nickname})
        print(f"📤 Posílám nickname: {self.nickname}")
    
    def handle_wait_lobby(self, data: dict):
        """Zpracuje WAIT_LOBBY zprávu."""
        print("⏳ Zpracovávám WAIT_LOBBY...")
        
        self.connected_players = data.get("current", 0)
        
        self.state = GameState.WAITING
        
        if self.game.state != State.LICITACE_TRUMF:
            self.game = Game(self.required_players, self.player_number)
        
        print(f"⏳ Čekám na hráče: {self.connected_players}/{self.required_players}")
    
    def handle_game_start(self, data: dict):
        """Zpracuje GAME_START zprávu."""
        print("🎮 Zpracovávám GAME_START...")
        print("🎮 HRA ZAČÍNÁ!")
    
        if not data:
            self.state = GameState.CONNECTING
            
        self.msg_handler.game_start_reader(data)
        
        # Přepnout do herního stavu
        self.state = GameState.PLAYING
        
        print("✅ Přepínám do PLAYING stavu")
    
    def handle_game_state(self, data: dict):
        """Zpracuje STATE zprávu - aktualizace stavu hry."""
        print("🔄 Zpracovávám STATE...")
        self.invalid = None
        self.msg_handler.state_reader(data)
    
    def handle_your_turn(self, data: dict):
        """Zpracuje YOUR_TURN zprávu - je můj tah."""
        print("🔔 Je můj tah!")
        self.game.active_player = True
        
    def handle_result(self, data: dict):
        """Zpracuje RESULT zprávu od serveru."""
        print("🔔 Přišla zpráva o výsledku!")
        self.msg_handler.game_result_reader(data)
        
    def handle_error(self, data: dict):
        """Zpracuje ERROR zprávu od serveru."""
        error_msg = data["message"]
        print(f"❌ CHYBA OD SERVERU: {error_msg}")
        msg = "ERROR: " + error_msg + "\n\n" + "Přepojuji do Lobby..."
        self.show_error_messages(msg)
        time.sleep(2)
        
        # Odpojit a vrátit do lobby
        self.client.disconnect()
        self.state = GameState.LOBBY
        
    def handle_client_data(self, data: dict):
        self.msg_handler.player_reader(data["client"])
        
    def handle_invalid(self, data: dict):
        self.invalid = data["data"]
    
    def handle_disconnect(self):
        """Callback při odpojení od serveru."""
        print("🔌 Odpojen od serveru!")
        self.state = GameState.LOBBY
    
    # ============================================================
    # AKCE OD UŽIVATELE - Připojení k serveru
    # ============================================================
    
    def connect_to_server(self):
        """Připojí se k serveru s údaji z lobby."""
        ip = self.ip_input.text
        port_str = self.port_input.text
        
        try:
            port = int(port_str)
        except ValueError:
            print("❌ Neplatný port!")
            return False
        
        print(f"🔌 Připojuji se na {ip}:{port}...")
        
        self.state = GameState.CONNECTING
        
        success = self.client.connect(ip, port)
        
        if not success:
            print("❌ Připojení selhalo!")
            self.state = GameState.LOBBY
            return False
        
        print("✅ Připojen!")
        return True
    
    # ============================================================
    # VYKRESLOVÁNÍ - HELPER FUNKCE
    # ============================================================
    
    def draw_text(self, text, font, color, x=None, y=None, center=False):
        """Vykreslí text s anti-aliasingem."""
        text_surface = font.render(text, True, color)
        text_rect = text_surface.get_rect()
        
        if center:
            text_rect.center = (WIDTH // 2, y if y else HEIGHT // 2)
        else:
            text_rect.topleft = (x if x else 0, y if y else 0)
        
        self.screen.blit(text_surface, text_rect)
        return text_rect
    
    def draw_cards(self, cards: list[Card], x=20, y=HEIGHT-140) -> list[tuple[pygame.Rect, str]]:
        """Vykreslí karty hráče s animací vysunutí."""
        if not cards:
            return []
        
        rects = []
        mouse_x, mouse_y = pygame.mouse.get_pos()
        space_between_cards = 80
        
        for i, card in enumerate(cards):
            rect = pygame.Rect(x + i * space_between_cards, y, CARD_WIDTH, CARD_HEIGHT)
            card_id = str(card)
            
            if card_id not in self.offsets:
                self.offsets[card_id] = 0
            
            # Animace vysunutí karty při hover
            if (rect.collidepoint((mouse_x, mouse_y)) and self.game.active_player and self.game.state != State.LICITACE_BETL_DURCH and
                self.game.state != State.LICITACE_DOBRY_SPATNY):
                target_offset = -20
            else:
                target_offset = 0
            
            current_offset = self.offsets[card_id]
            speed = 3
            
            if current_offset < target_offset:
                current_offset = min(current_offset + speed, target_offset)
            elif current_offset > target_offset:
                current_offset = max(current_offset - speed, target_offset)
            
            self.offsets[card_id] = current_offset
            
            # Stín karty
            shadow_rect = pygame.Rect(rect.x + 3, rect.y + current_offset + 3, 
                                     CARD_WIDTH, CARD_HEIGHT)
            pygame.draw.rect(self.screen, (0, 0, 0, 100), shadow_rect, border_radius=8)
            
            # Vykreslení karty
            image = card.get_image()
            self.screen.blit(image, (rect.x, rect.y + current_offset))
            
            # Zvýraznění při hover (pouze pokud je můj tah)
            if (rect.collidepoint((mouse_x, mouse_y)) and self.game.active_player and self.game.state != State.LICITACE_BETL_DURCH and
                self.game.state != State.LICITACE_DOBRY_SPATNY):
                highlight = pygame.Surface((CARD_WIDTH, CARD_HEIGHT), pygame.SRCALPHA)
                highlight.fill((255, 255, 255, 30))
                self.screen.blit(highlight, (rect.x, rect.y + current_offset))
            
            rects.append((pygame.Rect(rect.x, rect.y + current_offset, 
                                     CARD_WIDTH, CARD_HEIGHT), card_id))
        
        return rects
    
    def draw_played_cards(self, cards: list[Card]):
        """Vykreslí zahrané karty na stole."""
        space_between_cards = 60
        played_cards_only = [c for c in cards if c is not None]
        num_played_cards = len(played_cards_only)
        
        if num_played_cards == 0:
            return
        
        middle = (WIDTH // 2, HEIGHT // 2)
        y = middle[1] - (CARD_HEIGHT // 2)
        
        total_card_width = num_played_cards * CARD_WIDTH
        total_space_width = (num_played_cards - 1) * space_between_cards
        total_occupied_width = total_card_width + total_space_width
        start_x = (WIDTH - total_occupied_width) // 2
        current_x = start_x
        
        for i, c in enumerate(cards):
            if c is not None:
                rect = pygame.Rect(current_x, y, CARD_WIDTH, CARD_HEIGHT)
                
                # TODO: Zobrazit jméno hráče nebo číslo
                # TODO: určit vítěze
                #color = (0, 128, 0) if False else (0, 0, 0)  
                #text_width = self.font.size(f"Hráč {i}")
                #self.draw_text(f"Hráč {i}", self.font, color,
                #             (rect.x + (CARD_WIDTH // 2)) - (text_width[0] // 2), 
                #             rect.y - 30)
                
                image = c.get_image()
                self.screen.blit(image, (rect.x, rect.y))
                current_x += CARD_WIDTH + space_between_cards
    
    def draw_selection_buttons(self, labels, y_offset=50):
        """Vykreslí tlačítka výběru."""
        BUTTON_WIDTH, BUTTON_HEIGHT = 140, 50
        GAP = 20
        
        total_width = len(labels) * BUTTON_WIDTH + (len(labels) - 1) * GAP
        start_x = (WIDTH - total_width) // 2
        start_y = HEIGHT // 2 + y_offset
        
        button_rects = []
        mouse_pos = pygame.mouse.get_pos()
        
        for i, text in enumerate(labels):
            x = start_x + i * (BUTTON_WIDTH + GAP)
            rect = pygame.Rect(x, start_y, BUTTON_WIDTH, BUTTON_HEIGHT)
            
            # Hover efekt
            if rect.collidepoint(mouse_pos):
                color = YELLOW
                text_color = WHITE
            else:
                color = LIGHT_GRAY
                text_color = BLACK
            
            # Tlačítko
            pygame.draw.rect(self.screen, color, rect, border_radius=10)
            pygame.draw.rect(self.screen, DARK_GRAY, rect, 2, border_radius=10)
            
            # Text
            text_surf = self.font_medium.render(text, True, text_color)
            text_rect = text_surf.get_rect(center=rect.center)
            self.screen.blit(text_surf, text_rect)
            
            button_rects.append((rect, text))
        
        return button_rects
    
    # ============================================================
    # VYKRESLOVÁNÍ - HRA
    # ============================================================
    def show_active_player(self):
        if self.game.active_player:
            self.draw_text("Jste na tahu!", self.font_large, GREEN, y=20, center=True)
        else:
            self.draw_text("Čekejte na svůj tah...", self.font_large, GRAY, y=20, center=True)
            
    def show_game_results(self):
        self.draw_text(self.result, self.font_large, BLACK, center=True)
    
    def show_cards(self, count: int|str) -> list:
        return self.draw_cards(self.game.players[self.player_number].pick_cards(count))
    
    def show_invalid_move(self, msg: str):
        self.draw_text(msg, self.font_large, RED, y=HEIGHT-CARD_HEIGHT-20, center=True)
        
    def show_error_messages(self, msg: str):
        self.draw_text(msg, self.font_large, RED, y=20, center=True)
        
    def show_pick_trumph(self):
        if self.game.active_player:
            self.draw_text("Vyberte trumf!", self.font_large, GREEN, y=50, center=True)
        else:
            self.draw_text("Licitátor vybírá trumf.", self.font_large, GRAY, y=50, center=True)
         
    def show_removing_to_talon(self):
        if self.game.active_player:
            self.draw_text("Odhoďte kartu do talonu!", self.font_large, GREEN, y=50, center=True)
        else:
            self.draw_text("Hráč odhazuje karty do talonu.", self.font_large, GRAY, y=50, center=True)
        
    def show_mode_option(self):
        if self.game.active_player:
            self.draw_text("Vyberte hru!", self.font_large, GREEN, y=50, center=True)
            self.active_rects = self.draw_selection_buttons([mode.name for mode in Mode])
        else:
            self.draw_text("Hráč licituje.", self.font_large, GRAY, y=50, center=True)
        
    def show_first_bidding(self):
        mode_name = self.game.mode.name
        trumph = self.game.trumph
        self.draw_text(f"{mode_name} {trumph.name if trumph else ""}", self.font_large, GREEN, center=True)
        if self.game.active_player:
           self.active_rects = self.draw_selection_buttons(["Dobrý", "Špatný"])
        else:
            self.draw_text("Hráč licituje.", self.font_large, GRAY, y=50, center=True)

    def show_higher_game_option(self):
        mode_name = self.game.mode.name
        trumph = self.game.trumph
        self.draw_text(f"{mode_name} {trumph.name if trumph else ""}", self.font_large, GREEN, center=True)
        if self.game.active_player:
            self.draw_text("Vyber hru: ", self.font_large, GREEN, y=50, center=True)
            self.active_rects = self.draw_selection_buttons(["BETL", "DURCH"])
        else:
            self.draw_text("Hráč licituje.", self.font_large, GRAY, y=50, center=True)
    
    def show_second_bidding(self):
        mode_name = self.game.mode.name
        trumph = self.game.trumph
        self.draw_text(f"{mode_name} {trumph.name if trumph else ""}", self.font_large, GREEN, center=True)
        if self.game.active_player:
            self.active_rects = self.draw_selection_buttons(["Špatný"])
        else:
            self.draw_text("Hráč licituje.", self.font_large, GRAY, y=50, center=True)
            
    def show_players_info(self):
        height = 20
        self.draw_text("Lobby 1 - Přehled hráčů:",  self.font_medium, BLACK, x=20, y=height)
        # Moje jméno
        if self.player_number is not None:
            info_text = f"{self.nickname} (connected)"
            self.draw_text(info_text, self.font_small, BLUE, x=30, y=height+30)
            height += 50
            
            for i in range(self.required_players):
                if i != self.player_number:
                    self.draw_text(self.game.players[i].nickname + "(connected)", self.font_small, BLACK, x=30, y=height)
                    height += 20
                 
    def show_game(self):
        mode_name = self.game.mode.name
        trumph = self.game.trumph
        text = f"{mode_name} {trumph.name if trumph else ''}"
        text_width, text_height = self.font_medium.size(text)
        x = WIDTH - text_width - 20
        y = 20
        rect = pygame.Rect(x, y, text_width, text_height)
        pygame.draw.rect(self.screen, WHITE, rect)
        self.draw_text(text, self.font_medium, RED, x=x, y=y, center=False)
        pygame.draw.rect(self.screen, GRAY, rect, 1)

        if self.game.played_cards:
            self.draw_played_cards(self.game.played_cards)
            
        if self.invalid:
            self.show_invalid_move(self.invalid)
            
        if self.game.change_trick:
            if not self.waiting_for_trick:
                # 1. První frame - spustíme časovač
                self.waiting_for_trick = True
                self.trick_display_start = pygame.time.get_ticks()
            else:
                # 2. Další framy - kontrolujeme čas
                elapsed = pygame.time.get_ticks() - self.trick_display_start
                if elapsed >= 3000:  # 3 sekundy
                    # 3. Čas vypršel - pošleme zprávu
                    self.game.handle_trick()
                    self.client.send_message(MessageType.TRICK, {})
                    self.waiting_for_trick = False
            
        if not self.game.active_player:
            self.draw_text("Čekejte na váš tah.", self.font_large, GRAY, y=20, center=True)
        else:
            self.draw_text("Jste na tahu.", self.font_large, GREEN, y=20, center=True)
        
    def show_end_game(self):
        self.draw_text(self.game.result, self.font_large, BLACK, y=50, center=True)
        self.draw_text("Resetovat hru?", self.font_medium, RED, center=True)
        self.active_rects = self.draw_selection_buttons(["ANO", "NE"],)
        
    def show(self):
        """Vykreslí daný stav hry."""
        if self.game.state == State.LICITACE_TRUMF:
            self.show_pick_trumph()
        elif self.game.state == State.LICITACE_TALON:
            self.show_removing_to_talon()
        elif self.game.state == State.LICITACE_HRA:
            self.show_mode_option()  
        elif self.game.state == State.LICITACE_DOBRY_SPATNY:
            self.show_first_bidding()  
        elif self.game.state == State.LICITACE_BETL_DURCH:
            self.show_higher_game_option()
        elif self.game.state == State.HRA:
            self.show_game()  
        elif self.game.state == State.BETL:
            self.show_game()
        elif self.game.state == State.DURCH:
            self.show_game()
        else:
            self.show_end_game()
    
    # ============================================================
    # VYKRESLOVÁNÍ - Draw funkce pro různé stavy
    # ============================================================
    
    def draw_lobby(self):
        """Vykreslí lobby obrazovku."""
        self.screen.blit(self.background, (0, 0))
        
        # Nadpis s stínem
        self.draw_text("MARIÁŠ", self.title_font, DARK_GRAY, y=83, center=True)
        self.draw_text("MARIÁŠ", self.title_font, GOLD, y=80, center=True)
        self.draw_text("Online Multiplayer", self.font_medium, WHITE, y=150, center=True)
        
        # Input boxy a tlačítka
        self.ip_input.draw(self.screen)
        self.port_input.draw(self.screen)
        self.nickname_input.draw(self.screen)
        self.connect_button.draw(self.screen)
        self.quit_button.draw(self.screen)
    
    def draw_connecting(self):
        """Vykreslí obrazovku připojování."""
        self.screen.blit(self.background, (0, 0))
        
        dots = "." * ((pygame.time.get_ticks() // 500) % 4)
        self.draw_text(f"Připojuji{dots}", self.font_large, WHITE, 
                      y=HEIGHT // 2 - 50, center=True)
        
        self.draw_text("Probíhá příprava hry...", self.font_small, GRAY,
                      y=HEIGHT // 2 + 20, center=True)
    
    def draw_waiting(self):
        """Vykreslí obrazovku čekání na ostatní hráče."""
        self.screen.blit(self.background, (0, 0))
        
        self.draw_text("Čekám na hráče", self.font_large, WHITE,
                      y=HEIGHT // 2 - 100, center=True)
        
        player_text = f"{self.connected_players} / {self.required_players}"
        self.draw_text(player_text, self.font_large, GOLD,
                      y=HEIGHT // 2 - 20, center=True)
        
        if self.waiting_message:
            self.draw_text(self.waiting_message, self.font_small, GRAY,
                          y=HEIGHT // 2 + 50, center=True)
        
        dots = "." * ((pygame.time.get_ticks() // 500) % 4)
        self.draw_text(dots, self.font_medium, WHITE,
                      y=HEIGHT // 2 + 100, center=True)
        
        if self.player_number is not None:
            info_text = f"Hráč #{self.player_number}"
            self.draw_text(info_text, self.font_small, YELLOW,
                          y=HEIGHT - 50, center=True)
            
        self.quit_button.draw(self.screen)
    
    def draw_playing(self):
        """Vykreslí herní obrazovku - HLAVNÍ HRA."""
        self.screen.blit(self.background, (0, 0))
        
        self.show_players_info()
        
        if self.game.state == State.LICITACE_TRUMF and self.game.active_player:
            self.active_rects = self.show_cards(7)
        else:
            self.active_rects = self.show_cards("all")
            
        self.show()
        
    # ============================================================
    # EVENT HANDLING
    # ============================================================
    
    def handle_lobby_event(self, event):
        """Zpracuje události v lobby."""
        if self.ip_input.handle_event(event):
            return "connect"
        if self.port_input.handle_event(event):
            return "connect"
        if self.nickname_input.handle_event(event):
            return "connect"
        
        if self.connect_button.is_clicked(event):
            return "connect"
        if self.quit_button.is_clicked(event):
            pygame.quit()
            sys.exit()
        
        return None
    
    def handle_waiting_event(self, event):
        if self.quit_button.is_clicked(event):
            pygame.quit()
            sys.exit()
        
    
    def handle_playing_event(self, event):
        """Zpracuje herní události (klikání na karty nebo tlačítka)."""
        if event.type == pygame.MOUSEBUTTONDOWN and self.game.active_player:
            for rect, label in self.active_rects:
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
                    self.game.active_player = False
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
                if self.state == GameState.LOBBY:
                    action = self.handle_lobby_event(event)
                    if action == "connect":
                        self.connect_to_server()
                        
                elif self.state == GameState.WAITING:
                    action = self.handle_waiting_event(event)
                
                elif self.state == GameState.WAITING:
                    if action == "dissconect":
                        self.client.disconnect()
                
                elif self.state == GameState.PLAYING:
                    self.handle_playing_event(event)
            
            # Vykreslování podle stavu
            if self.state == GameState.LOBBY:
                self.draw_lobby()
            elif self.state == GameState.CONNECTING:
                self.draw_connecting()
            elif self.state == GameState.WAITING:
                self.draw_waiting()
            elif self.state == GameState.PLAYING:
                self.draw_playing()
            
            pygame.display.flip()
            self.clock.tick(60)
        
        # Cleanup
        if self.client.connected:
            self.client.disconnect()
        
        pygame.quit()
        sys.exit()


# ============================================================
# MAIN
# ============================================================

if __name__ == "__main__":
    gui = GUI()
    gui.run()