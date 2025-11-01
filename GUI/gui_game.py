import pygame
from card import CardSuits, Card, State, Mode
from GUI.game import Game
import sys

#root = tk.Tk()
WIDTH = 1200 #root.winfo_screenwidth()
HEIGHT = 700 #root.winfo_screenheight()
#root.destroy()
CARD_WIDTH, CARD_HEIGHT = 70, 100
IMG_DIR = "C:\\Users\\Lenka Jelinková\\Desktop\\UPS\\images\\wooden_table.jpg"

class guiGame:
    def __init__(self):
        self.screen = self.init_gui()
        self.font = pygame.font.SysFont("Arial", 28, bold=True)
        self.clock = pygame.time.Clock()
        self.middle = WIDTH // 2, HEIGHT // 2
        self.game = Game()
        self.active_rects = None
        self.offsets = {}  # { "A ♥": current_offset }
        
    def init_gui(self):
        """Inicializace GUI."""
        pygame.init()
        screen = pygame.display.set_mode((WIDTH, HEIGHT))
        image = pygame.image.load(IMG_DIR).convert_alpha()
        self.background = pygame.transform.scale(image, (WIDTH, HEIGHT))
        pygame.display.set_caption("Marias - pygame demo")
        return screen
    
    def reset_game(self):
        """Vyresetuje hru."""
        self.game = Game()

    def draw_text(self, text: str, x: int | None = None, y: int | None = None, color=(0, 0, 0)):
        """Obecná funkce pro vykreslení textu."""
        img = self.font.render(text, True, color)

        if x is None:
            x = self.text_placing_middle(text)[0]
        
        if y is None:
            y = self.text_placing_middle(text)[1]
            
        placing = (x, y)

        self.screen.blit(img, placing)
        
    def color_mapping(self, suit: CardSuits):
        """Namapuje barvu na jednotlivé karty."""
        color_map = {
            CardSuits.SRDCE: (255,0,0), # Červená
            CardSuits.KULE: (204,0,102),  # Karmínová
            CardSuits.ZALUDY: (0,0,0), # Černá
            CardSuits.LISTY: (0,155,0) # Tmavě zelená
        }
        return color_map.get(suit)
    
    def text_placing_middle(self, text: str) -> tuple[int, int]:
        """Vykreslý text na střed obrazovky."""
        text_w, text_h = self.font.size(text)
        text_x = (self.middle[0] - (text_w // 2)) 
        text_y = (self.middle[1] - (text_h // 2))
        return text_x, text_y
    
    def is_pointed(self, rect: pygame.Rect):
        """Zeptá se jestli je na daný objekt namířena myš."""
        mouse_x, mouse_y = pygame.mouse.get_pos()  # pozice myši
        
        if rect.collidepoint((mouse_x, mouse_y)):
            color = (150, 150, 150)  # světlejší šedá
        else:
            color = (255, 255, 255)  # tmavší šedá
            
        return color
    
    def draw_cards(self, cards: list[Card], x=20, y=HEIGHT-120) -> list[tuple[pygame.Rect, str]]:
        """Vykreslí karty hráče s animací vysunutí."""
        rects = []
        mouse_x, mouse_y = pygame.mouse.get_pos()
        for i, c in enumerate(cards):
            space_between_cards = 80
            rect = pygame.Rect(x + i*space_between_cards, y, CARD_WIDTH, CARD_HEIGHT)
            # identifikátor karty (aby měla svůj offset)
            card_id = str(c)
            if card_id not in self.offsets:
                self.offsets[card_id] = 0  # startovní offset

            # zjistíme, jestli je myš nad kartou
            if rect.collidepoint((mouse_x, mouse_y)):
                target_offset = -15  # vysunout o 15 px nahoru
            else:
                target_offset = 0  # základní pozice

            # plynulý přechod s dorovnáním
            current_offset = self.offsets[card_id]
            if current_offset < target_offset:
                current_offset += 2
                if current_offset > target_offset:
                    current_offset = target_offset
            elif current_offset > target_offset:
                current_offset -= 2
                if current_offset < target_offset:
                    current_offset = target_offset

            # uložíme nový offset
            self.offsets[card_id] = current_offset
            # vykreslíme obrázek karty s offsetem
            image = c.get_image()
            self.screen.blit(image, (rect.x, rect.y + current_offset))
            # přidáme rect posunutý o offset (kvůli kolizím!)
            rects.append((pygame.Rect(rect.x, rect.y + current_offset, CARD_WIDTH, CARD_HEIGHT), card_id))

        self.active_rects = rects
        return rects
    
    def draw_played_cards(self, cards: list[Card]):
        space_between_cards = 60
        played_cards_only = [c for c in cards if c is not None]
        num_played_cards = len(played_cards_only)
        if num_played_cards == 0:
            return []

        y = self.middle[1] - (CARD_HEIGHT // 2)
        total_card_width = num_played_cards * CARD_WIDTH
        total_space_width = (num_played_cards - 1) * space_between_cards
        total_occupied_width = total_card_width + total_space_width
        start_x = (WIDTH - total_occupied_width) // 2
        current_x = start_x

        for i, c in enumerate(cards):
            if c is not None:
                rect = pygame.Rect(current_x, y, CARD_WIDTH, CARD_HEIGHT)
                # Pokud tento hráč vyhrál, změň barvu textu
                color = (0, 128, 0) if self.game.trick_winner == i else (0, 0, 0)
                text_width = self.font.size(f"Hráč {i}")
                self.draw_text(f"Hráč {i}", (rect.x + (CARD_WIDTH // 2)) - (text_width[0] // 2), rect.y - 30, color=color)
                image = c.get_image()
                self.screen.blit(image, (rect.x, rect.y))
                current_x += CARD_WIDTH + space_between_cards
    
    def draw_selection_buttons(self, labels, y_offset=50):
        """Vykreslí tlačítka po licitování."""
        BUTTON_WIDTH, BUTTON_HEIGHT = 120, 40
        GAP = 30
        total_width = len(labels) * BUTTON_WIDTH + (len(labels) - 1) * GAP
        start_x = (WIDTH - total_width) // 2
        start_y = self.middle[1] + y_offset 
        button_rects = [] # Nový seznam pro vrácení
        self.active_cards = []
        for i, text in enumerate(labels):
            x = start_x + i * (BUTTON_WIDTH + GAP)
            rect = pygame.Rect(x, start_y, BUTTON_WIDTH, BUTTON_HEIGHT)
            pygame.draw.rect(self.screen, (80, 80, 80), rect, border_radius=10)
            pygame.draw.rect(self.screen, (200, 200, 200), rect, 2, border_radius=10)
            text_w, text_h = self.font.size(text)
            text_x = rect.x + (BUTTON_WIDTH - text_w) // 2
            text_y = rect.y + (BUTTON_HEIGHT - text_h) // 2
            self.draw_text(text, text_x, text_y)
            button_rects.append((rect, text))
            
        self.active_rects = button_rects
        return button_rects # Vracíme obdélníky
    
    def show_active_player(self):
        for player in self.game.players:
            if player == self.game.active_player:
                self.draw_text("Jsi na tahu", None, 20)
            else:
                self.draw_text(f"Hráč {self.game.active_player.nickname} je na tahu", None, 20)
    
    def show_cards(self, count: int|str):
        self.draw_cards(self.game.active_player.pick_cards(count))
    
    def show_pick_trumph(self):
        print("Licitátor vybírá trumf.")
         
    def show_removing_to_talon(self):
        print("Licitátor odhazuje karty do talonu.")
        
    def show_mode_option(self):
        print("Licitátor vybírá hru.")
        self.draw_selection_buttons([mode.name for mode in Mode])
        
    def show_first_bidding(self):
        mode_name = self.game.mode.name
        trumph = self.game.trumph
        self.draw_text(f"{mode_name} {trumph.name if trumph else ""}")
        self.draw_text(f"Hráč #{self.game.active_player.number}", None, 20)
        self.draw_cards(self.game.active_player.pick_cards("all"))
        self.draw_selection_buttons(["Dobrý", "Špatný"])

    def show_higher_game_option(self):
        print(f"Hráč #{self.game.active_player.number}", None, 20)
        self.draw_selection_buttons(["BETL", "DURCH"])
    
    def show_second_bidding(self):
        print(f"Hráč #{self.game.active_player.number}", None, 20)
        self.draw_selection_buttons(["Špatný"])
        
    def show_game(self):
        print(f"Hráč #{self.game.active_player.number}", None, 20)
        mode_name = self.game.mode.name
        trumph = self.game.trumph
        print(f"{mode_name} {trumph.name if trumph else ""}", None, 60)
        self.draw_played_cards(self.game.played_cards)
        self.draw_cards(self.game.active_player.pick_cards("all"))
        
    def show_end_game(self):
        self.draw_text(self.game.result, None, 20)
        self.draw_text("Resetovat hru?", None, 100)
        self.draw_selection_buttons(["ANO", "NE"],)
        
    def draw(self):
        """Vykreslí daný stav hry."""
        overlay = pygame.Surface((WIDTH, HEIGHT), pygame.SRCALPHA)  
        overlay.fill((255, 255, 255, 30))
        self.screen.blit(overlay, (0, 0))
        self.screen.blit(self.background, (0, 0))
        
        # if client.number == active_player.number
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
        
        pygame.display.flip() 
    
    def handle_event(self, event):
        """Mapuje funkce pro logiku podle stavů v reakci na jednotlivé události hráče."""
        if event.type == pygame.MOUSEBUTTONDOWN:
            selected_index = -1
            if self.game.check_stych():
                return
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