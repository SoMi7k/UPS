import pygame

from .Game.Game import Game, State
from .Game.Card import Card, Mode, card_mappping
from .Game.Player import Player, Hand
from .Client.ClientManager import ClientManager
from .View.GuiManager import GuiManager, Color

class GameManager:
    def __init__(self, required_players: int, clientManager: ClientManager, guiManager: GuiManager):
        self.client = clientManager
        self.gui = guiManager
        
        self.game = None
        self.rec = True
        
        self.required_players = required_players
        self.invalid = None
        self.waiting_for_trick = False
        self.offsets: dict = {}  # Pro animaci karet
        
        
    def set_game(self):
        self.game = Game(self.required_players, self.client.number)
    
    # ============================================================
    # VYKRESLOVÁNÍ - POMOCNÉ FUNKCE
    # ============================================================
    def draw_cards(self, cards: list[Card]) -> list[tuple[pygame.Rect, str]]:
        x = 20
        y = self.gui.height-140
        """Vykreslí karty hráče s animací vysunutí."""
        if not cards:
            return []
        
        rects = []
        mouse_x, mouse_y = pygame.mouse.get_pos()
        space_between_cards = 80
        
        for i, card in enumerate(cards):
            rect = pygame.Rect(x + i * space_between_cards, y, self.gui.card_width, self.gui.card_height)
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
                                     self.gui.card_width, self.gui.card_height)
            pygame.draw.rect(self.gui.screen, (0, 0, 0, 100), shadow_rect, border_radius=8)
            
            # Vykreslení karty
            image = card.get_image()
            self.gui.screen.blit(image, (rect.x, rect.y + current_offset))
            
            # Zvýraznění při hover (pouze pokud je můj tah)
            if (rect.collidepoint((mouse_x, mouse_y)) and self.game.active_player and self.game.state != State.LICITACE_BETL_DURCH and
                self.game.state != State.LICITACE_DOBRY_SPATNY):
                highlight = pygame.Surface((self.gui.card_width, self.gui.card_height), pygame.SRCALPHA)
                highlight.fill((255, 255, 255, 30))
                self.gui.screen.blit(highlight, (rect.x, rect.y + current_offset))
            
            rects.append((pygame.Rect(rect.x, rect.y + current_offset, 
                                     self.gui.card_width, self.gui.card_height), card_id))
        
        return rects
    
    def draw_played_cards(self, cards: list[Card]):
        """Vykreslí zahrané karty na stole."""
        space_between_cards = 60
        played_cards_only = [c for c in cards if c is not None]
        num_played_cards = len(played_cards_only)
        
        if num_played_cards == 0:
            return
        
        middle = (self.gui.width // 2, self.gui.height // 2)
        y = middle[1] - (self.gui.card_height // 2)
        
        total_width = num_played_cards * self.gui.card_width
        total_space_width = (num_played_cards - 1) * space_between_cards
        total_occupied_width = total_width + total_space_width
        start_x = (self.gui.width - total_occupied_width) // 2
        current_x = start_x
        
        for c in cards:
            if c is not None:
                rect = pygame.Rect(current_x, y, self.gui.card_width, self.gui.card_height)
                image = c.get_image()
                self.gui.screen.blit(image, (rect.x, rect.y))
                current_x += self.gui.card_width + space_between_cards
    
    def draw_selection_buttons(self, labels, y_offset=50):
        """Vykreslí tlačítka výběru."""
        BUTTON_WIDTH, BUTTON_HEIGHT = 140, 50
        GAP = 20
        
        total_width = len(labels) * BUTTON_WIDTH + (len(labels) - 1) * GAP
        start_x = (self.gui.width - total_width) // 2
        start_y = self.gui.height // 2 + y_offset
        
        button_rects = []
        mouse_pos = pygame.mouse.get_pos()
        
        for i, text in enumerate(labels):
            x = start_x + i * (BUTTON_WIDTH + GAP)
            rect = pygame.Rect(x, start_y, BUTTON_WIDTH, BUTTON_HEIGHT)
            
            # Hover efekt
            if rect.collidepoint(mouse_pos):
                color = Color.YELLOW
                text_color = Color.WHITE
            else:
                color = Color.LIGHT_GRAY
                text_color = Color.BLACK
            
            # Tlačítko
            pygame.draw.rect(self.gui.screen, color, rect, border_radius=10)
            pygame.draw.rect(self.gui.screen, Color.DARK_GRAY, rect, 2, border_radius=10)
            
            # Text
            text_surf = self.gui.font_medium.render(text, True, text_color)
            text_rect = text_surf.get_rect(center=rect.center)
            self.gui.screen.blit(text_surf, text_rect)
            
            button_rects.append((rect, text))
        
        return button_rects    
    
    def draw_playing(self):
        """Vykreslí herní obrazovku - HLAVNÍ HRA."""
        self.gui.screen.blit(self.gui.background, (0, 0))
        
        self.show_players_info()
        
        if self.game.state == State.LICITACE_TRUMF and self.game.active_player:
            self.active_rects = self.show_cards(7)
        else:
            self.active_rects = self.show_cards("all")
            
        self.show()
        
    # ============================================================
    # VYKRESLOVÁNÍ - STAVY HRY
    # ============================================================
    def show_active_player(self):
        if self.game.active_player:
            self.gui.draw_text("Jste na tahu!", self.gui.font_large, Color.GREEN, y=20, center=True)
        else:
            self.gui.draw_text("Čekejte na svůj tah...", self.gui.font_large, Color.GRAY, y=20, center=True)
            
    def show_game_results(self):
        self.gui.draw_text(self.game.result, self.gui.font_large, Color.BLACK, center=True)
    
    def show_cards(self, count: int|str) -> list:
        return self.draw_cards(self.game.players[self.client.number].pick_cards(count))
    
    def show_invalid_move(self, msg: str):
        self.gui.draw_text(msg, self.gui.font_large, Color.RED, y=self.gui.height-170, center=True)
        
    def show_error_messages(self, msg: str):
        self.gui.draw_text(msg, self.gui.font_large, Color.RED, y=20, center=True)
        
    def show_pick_trumph(self):
        if self.game.active_player:
            self.gui.draw_text("Vyberte trumf!", self.gui.font_large, Color.GREEN, y=50, center=True)
        else:
            self.gui.draw_text("Licitátor vybírá trumf.", self.gui.font_large, Color.GRAY, y=50, center=True)
         
    def show_removing_to_talon(self):
        if self.game.active_player:
            self.gui.draw_text("Odhoďte kartu do talonu!", self.gui.font_large, Color.GREEN, y=50, center=True)
        else:
            self.gui.draw_text("Hráč odhazuje karty do talonu.", self.gui.font_large, Color.GRAY, y=50, center=True)
        
    def show_mode_option(self):
        if self.game.active_player:
            self.gui.draw_text("Vyberte hru!", self.gui.font_large, Color.GREEN, y=50, center=True)
            self.active_rects = self.draw_selection_buttons([mode.name for mode in Mode])
        else:
            self.gui.draw_text("Hráč licituje.", self.gui.font_large, Color.GRAY, y=50, center=True)
        
    def show_first_bidding(self):
        mode_name = self.game.mode.name
        trumph = self.game.trumph
        self.gui.draw_text(f"{mode_name} {trumph.name if trumph else ''}", self.gui.font_large, Color.GREEN, center=True)
        if self.game.active_player:
           self.active_rects = self.draw_selection_buttons(["Dobrý", "Špatný"])
        else:
            self.gui.draw_text("Hráč licituje.", self.gui.font_large, Color.GRAY, y=50, center=True)

    def show_higher_game_option(self):
        mode_name = self.game.mode.name
        trumph = self.game.trumph
        self.gui.draw_text(f"{mode_name} {trumph.name if trumph else ''}", self.gui.font_large, Color.GREEN, center=True)
        if self.game.active_player:
            self.gui.draw_text("Vyber hru: ", self.gui.font_large, Color.GREEN, y=50, center=True)
            self.active_rects = self.draw_selection_buttons(["BETL", "DURCH"])
        else:
            self.gui.draw_text("Hráč licituje.", self.gui.font_large, Color.GRAY, y=50, center=True)
    
    def show_second_bidding(self):
        mode_name = self.game.mode.name
        trumph = self.game.trumph
        self.gui.draw_text(f"{mode_name} {trumph.name if trumph else ''}", self.gui.font_large, Color.GREEN, center=True)
        if self.game.active_player:
            self.active_rects = self.draw_selection_buttons(["Špatný"])
        else:
            self.gui.draw_text("Hráč licituje.", self.gui.font_large, Color.GRAY, y=50, center=True)
            
    def show_players_info(self):
        height = 20
        self.gui.draw_text("Lobby 1 - Přehled hráčů:",  self.gui.font_medium, Color.BLACK, x=20, y=height)
        # Moje jméno
        if self.client.number is not None:
            info_text = f"{self.client.nickname}"
            self.gui.draw_text(info_text, self.gui.font_small, Color.BLUE, x=30, y=height+30)
            height += 50
            
            for i in range(self.required_players):
                if i != self.client.number:
                    self.gui.draw_text(self.game.players[i].nickname, self.gui.font_small, Color.BLACK, x=30, y=height)
                    height += 20
                 
    def show_game(self):
        mode_name = self.game.mode.name
        trumph = self.game.trumph
        text = f"{mode_name} {trumph.name if trumph else ''}"
        text_width, text_height = self.gui.font_medium.size(text)
        x = self.gui.width - text_width - 20
        y = 20
        rect = pygame.Rect(x, y, text_width, text_height)
        pygame.draw.rect(self.gui.screen, Color.WHITE, rect)
        self.gui.draw_text(text, self.gui.font_medium, Color.RED, x=x, y=y, center=False)
        pygame.draw.rect(self.gui.screen, Color.GRAY, rect, 1)

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
                    self.client.send_empty_trick()
                    self.waiting_for_trick = False
            
        if not self.game.active_player:
            self.gui.draw_text("Čekejte na váš tah.", self.gui.font_large, Color.GRAY, y=20, center=True)
        else:
            self.gui.draw_text("Jste na tahu.", self.gui.font_large, Color.GREEN, y=20, center=True)
        
    def show_end_game(self):
        self.gui.draw_text(self.game.result, self.gui.font_large, Color.BLACK, y=50, center=True)
        self.gui.draw_text("Resetovat hru?", self.gui.font_medium, Color.RED, center=True)
        self.active_rects = self.draw_selection_buttons(["ANO", "NE"],)
        
    def show(self):
        """Vykreslí daný stav hry."""
        if self.game.state == State.LICITACE_TRUMF:
            self.show_pick_trumph()
        elif self.game.state == State.LICITACE_TALON:
            self.show_removing_to_talon()
        elif self.game.state == State.LICITACE_HRA:
            #if self.client.number == 1 and self.rec:
            #    self.rec = False
            #    self.client.sock.close()
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
    # DESERIALIZACE
    # ============================================================        
    def card_reader(self, card: str) -> Card:
        new_card = card_mappping(card)
        return new_card
    
    def player_reader(self, player_data: list):
        nick_numb = player_data[0].split("-")
        number = int(nick_numb[0])
        nickname = nick_numb[1]
        hand = Hand()
        cards = player_data[1][:-1].split(":")
        print(f"SER CARDS: {cards}")
        for card in cards:
            if not card:
                continue
            new_card = card_mappping(card)
            hand.add_card(new_card)
        player = Player(number, nickname, hand)

        self.game.add_player(player)
    
    def state_reader(self, data: list):
        # state|stateChanged|gameStarted|mode|trumph|isPlayedCards|cards|change_trick
        if int(data[1]):
            state_num = int(data[0])
            try:
                self.game.state  = State(state_num)
            except ValueError:
                print("Stav hry se špatně načetl")
                self.game.state  = State.CHYBOVÝ_STAV

        if len(data) > 2 and int(data[2]):
            self.game.init_mode(int(data[3]))
            self.game.init_trump(data[4])
            if len(data) > 5 and int(data[5]):
                if len(data) > 7 and int(data[7]):
                    self.game.change_trick = True
                cards = data[6][:-1].split(":")
                print(f"SER CARDS: {cards}")
                for card in cards:
                    new_card = self.card_reader(card)
                    self.game.add_played_card(new_card)
        
    def game_start_reader(self, data: list):
        self.player_reader(data)
        self.game.init_players(data)
        
    def game_result_reader(self, data: list):
        score = data[0]
        score_tuple = score.split(":")
        self.game.evaluate_result(tuple(score_tuple))