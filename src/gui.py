import pygame
from cards import CardSuits, Card, State, Mode
from game import Game
import sys

WIDTH, HEIGHT = 800, 600
CARD_WIDTH, CARD_HEIGHT = 50, 80

class GUI:
    def __init__(self):
        self.screen = self.init_gui()
        self.font = pygame.font.SysFont("Arial", 24)
        self.clock = pygame.time.Clock()
        self.middle = WIDTH // 2, HEIGHT // 2
        self.game = Game()
        self.active_rects = None
        
    def init_gui(self):
        pygame.init()
        screen = pygame.display.set_mode((WIDTH, HEIGHT))
        screen.fill((0,0,255))
        pygame.display.set_caption("Marias - pygame demo")
        return screen
    
    def reset_game(self):
        self.game = Game()

    def draw_text(self, text: str, x: int | None = None, y: int | None = None, color=(0, 0, 0)):
        img = self.font.render(text, True, color)

        if x is None or y is None:
            # výpočet středu, pokud nebyly předány souřadnice
            placing = self.text_placing_middle(text)
        else:
            placing = (x, y)

        self.screen.blit(img, placing)
        
    def color_mapping(self, suit: CardSuits):
        color_map = {
            CardSuits.SRDCE: (255,0,0), # Červená
            CardSuits.KULE: (204,0,102),  # Karmínová
            CardSuits.ZALUDY: (0,0,0), # Černá
            CardSuits.LISTY: (0,155,0) # Tmavě zelená
        }
        return color_map.get(suit)
    
    def text_placing_middle(self, text: str) -> tuple[int, int]:
        text_w, text_h = self.font.size(text)
        text_x = (self.middle[0] - (text_w // 2)) 
        text_y = (self.middle[1] - (text_h // 2))
        return text_x, text_y
    
    def is_pointed(self, rect: pygame.Rect):
        mouse_x, mouse_y = pygame.mouse.get_pos()  # pozice myši
        
        if rect.collidepoint((mouse_x, mouse_y)):
            color = (150, 150, 150)  # světlejší šedá
        else:
            color = (255, 255, 255)  # tmavší šedá
            
        return color
    
    def draw_played_cards(self, cards: list[Card]):
        """
        Vykreslí odehrané karty na střed okna (horizontálně i vertikálně).
        """
        # Mezera mezi kartami (stejná jako v draw_cards)
        space_between_cards = 60
        # Určíme počet karet, které se budou vykreslovat (ty, co nejsou None)
        played_cards_only = [c for c in cards if c is not None]
        num_played_cards = len(played_cards_only)
        if num_played_cards == 0:
            return [] # Není co kreslit

        # 1. Výpočet pozice Y (vertikální střed)
        # Použijeme self.middle[1] pro svislý střed okna a posuneme o polovinu výšky karty.
        y = self.middle[1] - (CARD_HEIGHT // 2) 

        # 2. Výpočet pozice X (horizontální střed)
        # Vypočítáme celkovou šířku zabranou kartami a mezerami
        total_card_width = num_played_cards * CARD_WIDTH
        total_space_width = (num_played_cards - 1) * space_between_cards
        total_occupied_width = total_card_width + total_space_width
        
        # Počáteční X pozice pro centrování
        start_x = (WIDTH - total_occupied_width) // 2
        
        # Počítadlo pro posun při vykreslování
        current_x = start_x
        
        for i, c in enumerate(cards):
            if c is not None:
                # Pozice X pro aktuální kartu
                x = current_x
                
                rect = pygame.Rect(x, y, CARD_WIDTH, CARD_HEIGHT)
                
                # Vykreslení jména hráče (nad kartou) - např. "Hráč 0"
                self.draw_text(f"Hráč {i}", rect.x, rect.y - 20, color=(0, 0, 0))

                # Barva pozadí
                color = (255, 255, 255)
                
                pygame.draw.rect(self.screen, color, rect)
                pygame.draw.rect(self.screen, (0, 0, 0), rect, 2) 
                
                # Vykreslení textu karty
                text_w, text_h = self.font.size(str(c))
                text_x = rect.x + (CARD_WIDTH - text_w) // 2
                text_y = rect.y + (CARD_HEIGHT - text_h) // 2
                self.draw_text(str(c), text_x + 10, text_y, color=self.color_mapping(c.suit))
                
                # Posun na další kartu (karta + mezera)
                current_x += CARD_WIDTH + space_between_cards

    def draw_cards(self, cards: list[Card], x=20, y=HEIGHT-100) -> list[tuple[pygame.Rect, str]]:
        rects = []
        for i, c in enumerate(cards):
            space_between_cards = 60
            rect = pygame.Rect(x + i*space_between_cards, y, CARD_WIDTH, CARD_HEIGHT)
            color = self.is_pointed(rect)
            pygame.draw.rect(self.screen, color, rect)
            pygame.draw.rect(self.screen, (0,0,0), rect, 2) 
            text_w, text_h = self.font.size(str(c))
            text_x = rect.x + (CARD_WIDTH - text_w) // 2
            text_y = rect.y + (CARD_HEIGHT - text_h) // 2
            self.draw_text(str(c), text_x, text_y, color=self.color_mapping(c.suit))
            rects.append((rect, str(c)))
            
        self.active_rects = rects
        return rects
    
    def draw_selection_buttons(self, labels, y_offset=50):
        """Vykreslí tlačítka po licitování."""
        BUTTON_WIDTH, BUTTON_HEIGHT = 100, 40
        GAP = 20
        total_width = len(labels) * BUTTON_WIDTH + (len(labels) - 1) * GAP
        start_x = (WIDTH - total_width) // 2
        start_y = self.middle[1] + y_offset 
        button_rects = [] # Nový seznam pro vrácení
        self.active_cards = []
        for i, text in enumerate(labels):
            x = start_x + i * (BUTTON_WIDTH + GAP)
            rect = pygame.Rect(x, start_y, BUTTON_WIDTH, BUTTON_HEIGHT)
            color = self.is_pointed(rect)
            pygame.draw.rect(self.screen, color, rect) # Šedé pozadí
            pygame.draw.rect(self.screen, color, rect, 2)
            text_w, text_h = self.font.size(text)
            text_x = rect.x + (BUTTON_WIDTH - text_w) // 2
            text_y = rect.y + (BUTTON_HEIGHT - text_h) // 2
            self.draw_text(text, text_x, text_y)
            button_rects.append((rect, text))
            
        self.active_rects = button_rects
        return button_rects # Vracíme obdélníky
    
    def draw_state_1(self):
        self.draw_text("Licitátor vybírá trumf.", 300, 20)
        self.draw_cards(self.game.active_player.pick_cards(7))
         
    def draw_state_2(self):
        self.draw_text("Licitátor odhazuje karty do talonu.", 300, 20)
        self.draw_cards(self.game.active_player.pick_cards("all"))
        
    def draw_state_3(self):
        self.draw_text("Licitátor vybírá hru.", 300, 20)
        self.draw_cards(self.game.active_player.pick_cards("all"))
        self.draw_selection_buttons([mode.name for mode in Mode])
        
    def draw_state_4(self):
        self.draw_text(f"{self.game.game_logic.mode.name} {self.game.game_logic.trumph or ""}")
        self.draw_text(f"Hráč #{self.game.active_player.number}", 300, 20)
        self.draw_cards(self.game.active_player.pick_cards("all"))
        self.draw_selection_buttons(["Dobrý", "Špatný"])

    def draw_state_5(self):
        self.draw_text(f"Hráč #{self.game.active_player.number}", 300, 20)
        self.draw_cards(self.game.active_player.pick_cards("all"))
        self.draw_selection_buttons(["BETL", "DURCH"])
    
    def draw_state_6(self):
        self.draw_text(f"Hráč #{self.game.active_player.number}", 300, 20)
        self.draw_selection_buttons(["Špatný"])
        self.draw_cards(self.game.active_player.pick_cards("all"))
        
    def draw_state_7(self):
        self.draw_text(f"Hráč #{self.game.active_player.number}", 300, 20)
        self.draw_text(f"{self.game.game_logic.mode.name} {self.game.game_logic.trumph or ""}", 300, 40)
        self.draw_played_cards(self.game.played_cards)
        self.draw_cards(self.game.active_player.pick_cards("all"))
        
    def draw_state_8(self):
        self.draw_text(self.game.result_str, 300, 20)
        self.draw_text("Resetovat hru?", 300, 100)
        self.draw_selection_buttons(["ANO", "NE"],)
        
    def draw(self):
        self.screen.fill((0, 0, 255))
        
        if self.game.state == State.ROZDÁNÍ_KARET:
            self.game.deal_cards()
            self.game.state = State.LICITACE_TRUMF
        elif self.game.state == State.LICITACE_TRUMF:
            self.draw_state_1()
        elif self.game.state == State.LICITACE_TALON:
            self.draw_state_2()
        elif self.game.state == State.LICITACE_HRA:
            self.draw_state_3()  
        elif self.game.state == State.LICITACE_DOBRY_SPATNY:
            self.draw_state_4()  
        elif self.game.state == State.LICITACE_BETL_DURCH:
            self.draw_state_5()
        elif self.game.state == State.HRA:
            self.draw_state_7()  
        elif self.game.state == State.BETL:
            self.draw_state_7()
        elif self.game.state == State.DURCH:
            self.draw_state_7()
        else:
            self.draw_state_8()
        
        pygame.display.flip() 
    
    def handle_event(self, event):
        if event.type == pygame.MOUSEBUTTONDOWN:
            selected_index = -1
            for i, r in enumerate(self.active_rects):
                if r[0].collidepoint(event.pos):
                    selected_index = i
                    print(f"Hráč vybral {r[1]}")
            
            choosed_label = self.active_rects[selected_index]
            player = self.game.active_player
            if selected_index > -1:
                match self.game.state:
                    case State.LICITACE_TRUMF:
                        self.game.game_state_1(player.hand.cards[selected_index])
                    case State.LICITACE_TALON:
                        self.game.game_state_2(player.hand.cards[selected_index])
                    case State.LICITACE_HRA:
                        self.game.game_state_3(choosed_label[1])
                    case State.LICITACE_DOBRY_SPATNY:
                        self.game.game_state_4(choosed_label[1])
                    case State.LICITACE_BETL_DURCH:
                        self.game.game_state_5(choosed_label[1])
                    case State.HRA:
                        self.game.game_state_6(player.hand.cards[selected_index])
                    case State.BETL:
                        self.game.game_state_7(player.hand.cards[selected_index])
                    case State.DURCH:
                        self.game.game_state_7(player.hand.cards[selected_index])
                    case State.END:
                        if self.game.game_state_8(r[1]):
                            self.reset_game()
                        else:
                            pygame.quit()
                            sys.exit()
                    case _:
                        raise Exception("Error State")
                        
if __name__ == "__main__":
    game = Game()
    gui = GUI()

    while True:
        gui.draw()
        
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                pygame.quit()
                sys.exit()
            gui.handle_event(event)
            
        gui.clock.tick(30)