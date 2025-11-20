import pygame
from pygame import Surface
from card import Card, State, Mode
from game import Game
from msg_handler import msgHandler

WHITE = (255, 255, 255)
BLACK = (0, 0, 0)
GRAY = (200, 200, 200)
DARK_GRAY = (100, 100, 100)
LIGHT_GRAY = (240, 240, 240)
GREEN = (34, 139, 34)
YELLOW = (255, 255, 0)
DARK_GREEN = (0, 100, 0)
RED = (220, 20, 60)
BLUE = (70, 130, 180)
GOLD = (255, 215, 0)

#root = tk.Tk()
WIDTH = 1200 #root.winfo_screenwidth()
HEIGHT = 700 #root.winfo_screenheight()
#root.destroy()
CARD_WIDTH, CARD_HEIGHT = 70, 100
IMG_DIR = "C:\\Users\\Lenka Jelinková\\Desktop\\UPS\\images\\wooden_table.jpg"

class guiGame:
    def __init__(self, screen: Surface, game: Game, msg_handler: msgHandler, font):
        self.screen = screen
        self.msg_handler = msg_handler
        self.game = game
        
        self.font = font
        self.middle = WIDTH // 2, HEIGHT // 2
        self.active_rects = None
        self.offsets = {}
        
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
            card_id = self.msg_handler.card_wrapper(card)
            
            if card_id not in self.offsets:
                self.offsets[card_id] = 0
            
            # Animace vysunutí karty při hover
            if rect.collidepoint((mouse_x, mouse_y)) and self.game.active_player:
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
            if rect.collidepoint((mouse_x, mouse_y)) and self.game.active_player:
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
                color = (0, 128, 0) if False else (0, 0, 0)  # TODO: určit vítěze
                text_width = self.font.size(f"Hráč {i}")
                self.draw_text(f"Hráč {i}", self.font, color,
                             (rect.x + (CARD_WIDTH // 2)) - (text_width[0] // 2), 
                             rect.y - 30)
                
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
                color = BLUE
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
    
    def show_active_player(self):
        if self.game.active_player:
            self.draw_text("Jste na tahu!", self.font_large, GREEN, y=20, center=True)
        else:
            self.draw_text("Čekejte na svůj tah...", self.font_large, GRAY, y=20, center=True)
    
    def show_cards(self, count: int|str) -> list:
        return self.draw_cards(self.game.players[self.player_number].pick_cards(count))
    
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
        
    def show_game(self):
        mode_name = self.game.mode.name
        trumph = self.game.trumph
        text = f"{mode_name} {trumph.name if trumph else ""}"
        self.draw_text(text, self.font_medium, RED, x=800, y=20, center=False)
        rect = pygame.Rect(800, 20, self.font_medium.size(text)[0], self.font_medium.size(text)[1])
        pygame.draw.rect(self.screen, WHITE, rect, 1)
        if self.played_cards:
            self.draw_played_cards(self.game.played_cards)
            
        if not self.game.active_player:
            self.draw_text("Čekejte na váš tah.", self.font_large, GRAY, y=20, center=True)
        
    def show_end_game(self):
        self.draw_text(self.game.result, self.font_large, RED, y=20, center=True)
        self.draw_text("Resetovat hru?", self.font_large, RED, center=True)
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