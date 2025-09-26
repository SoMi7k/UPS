# marias_pygame.py
import pygame
import sys
import random

from cards import CardRanks, CardSuits
#from src.player import Player
#import src.game

SUITS = ['♥','♦','♣','♠']
RANKS = ['A','K','F','S','10','9','8','7']

# ----------- Herní model (zjednodušený) -----------
class Card:
    """
    Class Card

    Třída, která incializuje jednotlivé karty.
    Kartu tvoří číslo, neboli její hodnota (CardRanks) a barva (CardSuits)
    """

    def __init__(self, rank: str, suit: CardSuits):
        self.rank = rank
        self.suit = suit
        
    def __eq__(self, other):
        """Porovná mezi sebou 2 karty."""
        if isinstance(other, Card):
            return self.rank == other.rank and self.suit == other.suit
        return False

    def __str__(self):
        """Vypíše kartu."""
        return f"{self.rank} {self.suit.value}"

class Deck:
    """
    Class Deck

    Třída, která inicializuje balíček karet.
    Zde jsou všechny karty, celkový počet 32, které tato třída náhodně smíchá do jednoho pole (seznamu).
    """
    CARDS_NUMBER = 32

    def __init__(self):
        self.cards = self.shuffle()
        self.card_index = 0

    def initialize_deck(self) -> list:
        """
        Metoda vytvoří standardní balíček 32 karet.
        """
        return [Card(rank, suit) for suit in CardSuits for rank in RANKS]
        
    def shuffle(self) -> list:
        """
        Metoda náhodně zamíchá balíček karet.
        """
        deck = self.initialize_deck() # Zajistí, že balíček je kompletní
        random.shuffle(deck)
        self.card_index = 0 # Resetuje index po zamíchání
        return deck

    def deal(self) -> Card|None:
        return self.cards.pop() if self.cards else None

class Player:
    def __init__(self, number):
        self.number = number
        self.cards = []

    def add_card(self, c):
        self.cards.append(c)

    def play_card(self, index):
        return self.cards.pop(index)
    
# ----------- Pygame GUI -----------
pygame.init()
WIDTH, HEIGHT = 800, 600
screen = pygame.display.set_mode((WIDTH, HEIGHT))
pygame.display.set_caption("Marias - pygame demo")
font = pygame.font.SysFont("Arial", 24)

# Hra
players = [Player(0), Player(1), Player(2)]
deck = Deck()
while deck.cards:
    for p in players:
        if deck.cards:
            p.add_card(deck.deal())

table_cards: list[Card] = []

def draw_text(text, x, y, color=(0,0,0)):
    img = font.render(text, True, color)
    screen.blit(img, (x, y))

def color_mapping(suit: CardSuits):
    color_map = {
    CardSuits.SRDCE: (255,0,0), # Červená
    CardSuits.KULE: (204,0,102),  # Karmínová
    CardSuits.ZALUDY: (0,0,0), # Černá
    CardSuits.LISTY: (0,155,0) # Tmavě zelená
    }
        
    return color_map.get(suit)

def draw_cards(cards: list[Card], x: int, y: int, selected_index=None):
    rects = []
    for i, c in enumerate(cards):
        rect = pygame.Rect(x + i*60, y, 50, 80)
        color = (200,200,255) if i == selected_index else (255,255,255)
        pygame.draw.rect(screen, color, rect)
        pygame.draw.rect(screen, (0,0,0), rect, 2) 
        text_w, text_h = font.size(str(c)) 
        text_x = rect.x + (50 - text_w) // 2
        text_y = rect.y + (80 - text_h) // 2
        draw_text(str(c), text_x, text_y, color=color_mapping(c.suit))
        rects.append(rect)
    return rects

current_player = 0
selected_index = None

clock = pygame.time.Clock()

def draw():
    screen.fill((0,150,0))  # zelený stůl

    # Karty hráče dole
    player = players[current_player]
    draw_text(f"Hráč #{player.number} - tvoje karty:", 20, HEIGHT-150)
    rects = draw_cards(player.cards, 20, HEIGHT-100, selected_index)

    # Karty na stole
    draw_text("Na stole:", 20, 20)
    draw_cards(table_cards, 20, 80)
    return rects, player

# ----------- Main loop -----------
while True:
    rects, player = draw()

    pygame.display.flip()

    for event in pygame.event.get():
        if event.type == pygame.QUIT:
            pygame.quit()
            sys.exit()

        elif event.type == pygame.MOUSEBUTTONDOWN:
            for i, r in enumerate(rects):
                if r.collidepoint(event.pos):
                    selected_index = i

        elif event.type == pygame.KEYDOWN:
            if event.key == pygame.K_RETURN and selected_index is not None:
                card = player.play_card(selected_index)
                table_cards.append(card)
                selected_index = None
                current_player = (current_player + 1) % len(players)

    clock.tick(30)
