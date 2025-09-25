# marias_pygame.py
import pygame
import sys
import random

# ----------- Herní model (zjednodušený) -----------
SUITS = ["♥", "♦", "♣", "♠"]
RANKS = ["a", "k", "q", "j", "10", "9", "8", "7"]

class Card:
    def __init__(self, rank, suit):
        self.rank = rank
        self.suit = suit

    def __str__(self):
        return f"{self.rank}{self.suit}"

class Deck:
    def __init__(self):
        self.cards = [Card(r, s) for s in SUITS for r in RANKS]
        random.shuffle(self.cards)

    def deal(self):
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

table_cards = []

def draw_text(text, x, y, color=(0,0,0)):
    img = font.render(text, True, color)
    screen.blit(img, (x, y))

def draw_cards(cards, x, y, selected_index=None):
    rects = []
    for i, c in enumerate(cards):
        rect = pygame.Rect(x + i*60, y, 50, 80)
        color = (200,200,255) if i == selected_index else (255,255,255)
        pygame.draw.rect(screen, color, rect)
        pygame.draw.rect(screen, (0,0,0), rect, 2)
        draw_text(str(c), rect.x+5, rect.y+30)
        rects.append(rect)
    return rects

current_player = 0
selected_index = None

clock = pygame.time.Clock()

# ----------- Main loop -----------
while True:
    screen.fill((0,150,0))  # zelený stůl

    # Karty hráče dole
    player = players[current_player]
    draw_text(f"Hráč #{player.number} - tvoje karty:", 20, HEIGHT-120)
    rects = draw_cards(player.cards, 20, HEIGHT-100, selected_index)

    # Karty na stole
    draw_text("Na stole:", 20, 50)
    draw_cards(table_cards, 20, 80)

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
