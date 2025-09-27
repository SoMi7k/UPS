from enum import Enum
import random
import pygame
import os

class State(Enum):
    ROZDÁNÍ_KARET = 0
    LICITACE_TRUMF = 1
    LICITACE_TALON = 2
    LICITACE_HRA = 3
    LICITACE_DOBRY_SPATNY = 4
    LICITACE_BETL_DURCH = 5
    HRA = 6
    BETL = 7
    DURCH = 7
    END = 8

class Mode(Enum):
    HRA = 1
    #SEDMA = 2
    BETL = 4
    DURCH = 5
    
class CardRanks(Enum):
    # Příklad hodnot, nahraďte je skutečnými hodnotami karet
    A = 8
    X = 7
    K = 6
    Q = 5
    J = 4
    IX = 3
    VIII = 2
    VII = 1
    
class CardSuits(Enum):
    # Příklad barev, nahraďte je skutečnými barvami karet
    SRDCE = '♥'
    KULE = '♦'
    ZALUDY = '♣'
    LISTY = '♠'
    
# Lists for mapping
SUIT_LIST = ['♥', '♦', '♣', '♠']
RANK_LIST = ['a', 'k', 'q', 'j', '10', '9', '8', '7']

# Mapování barvy    
SUITE_MAP = {
    '♥': CardSuits.SRDCE,
    '♦': CardSuits.KULE,
    '♣': CardSuits.ZALUDY,
    '♠': CardSuits.LISTY
}

# Mapování hodnoty
RANK_MAP = {
    'a': CardRanks.A,
    'k': CardRanks.K,
    'q': CardRanks.Q,
    'j': CardRanks.J,
    '10': CardRanks.X,
    '9': CardRanks.IX,
    '8': CardRanks.VIII,
    '7': CardRanks.VII
}
    
CARD_IMAGES = {
    (CardRanks.A, CardSuits.ZALUDY): "images/acorn-ace.png",
    (CardRanks.K, CardSuits.ZALUDY): "images/acorn-king.png",
    (CardRanks.Q, CardSuits.ZALUDY): "images/acorn-ober.png",
    (CardRanks.J, CardSuits.ZALUDY): "images/acorn-unter.png",
    (CardRanks.X, CardSuits.ZALUDY): "images/acorn-ten.png",
    (CardRanks.IX, CardSuits.ZALUDY): "images/acorn-nine.png",
    (CardRanks.VIII, CardSuits.ZALUDY): "images/acorn-eight.png",
    (CardRanks.VII, CardSuits.ZALUDY): "images/acorn-seven.png",
    
    (CardRanks.A, CardSuits.SRDCE): "images/heart-ace.png",
    (CardRanks.K, CardSuits.SRDCE): "images/heart-king.png",
    (CardRanks.Q, CardSuits.SRDCE): "images/heart-ober.png",
    (CardRanks.J, CardSuits.SRDCE): "images/heart-unter.png",
    (CardRanks.X, CardSuits.SRDCE): "images/heart-ten.png",
    (CardRanks.IX, CardSuits.SRDCE): "images/heart-nine.png",
    (CardRanks.VIII, CardSuits.SRDCE): "images/heart-eight.png",
    (CardRanks.VII, CardSuits.SRDCE): "images/heart-seven.png",
    
    (CardRanks.A, CardSuits.KULE): "images/bell-ace.png",
    (CardRanks.K, CardSuits.KULE): "images/bell-king.png",
    (CardRanks.Q, CardSuits.KULE): "images/bell-ober.png",
    (CardRanks.J, CardSuits.KULE): "images/bell-unter.png",
    (CardRanks.X, CardSuits.KULE): "images/bell-ten.png",
    (CardRanks.IX, CardSuits.KULE): "images/bell-nine.png",
    (CardRanks.VIII, CardSuits.KULE): "images/bell-eight.png",
    (CardRanks.VII, CardSuits.KULE): "images/bell-seven.png",
    
    (CardRanks.A, CardSuits.LISTY): "images/leaf-ace.png",
    (CardRanks.K, CardSuits.LISTY): "images/leaf-king.png",
    (CardRanks.Q, CardSuits.LISTY): "images/leaf-ober.png",
    (CardRanks.J, CardSuits.LISTY): "images/leaf-unter.png",
    (CardRanks.X, CardSuits.LISTY): "images/leaf-ten.png",
    (CardRanks.IX, CardSuits.LISTY): "images/leaf-nine.png",
    (CardRanks.VIII, CardSuits.LISTY): "images/leaf-eight.png",
    (CardRanks.VII, CardSuits.LISTY): "images/leaf-seven.png",
}

class Card:
    """
    Class Card

    Třída, která incializuje jednotlivé karty.
    Kartu tvoří číslo, neboli její hodnota (CardRanks) a barva (CardSuits)
    """

    def __init__(self, rank: CardRanks, suit: CardSuits):
        self.rank = rank
        self.suit = suit
        
    def __eq__(self, other):
        """Porovná mezi sebou 2 karty."""
        if isinstance(other, Card):
            return self.rank == other.rank and self.suit == other.suit
        return False

    def __str__(self):
        """Vrátí string dané karty."""
        return f"{self.rank.name} {self.suit.value}"

    def __repr__(self):
        """Metoda pro formální reprezentaci objektu, užitečná pro debugování."""
        return f"Card(rank={self.rank}, suit={self.suit})"
    
    def get_image(self) -> pygame.Surface:
        """Vrátí pygame Surface s obrázkem karty."""
        path = CARD_IMAGES[(self.rank, self.suit)]
        
        IMG_DIR = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), path)
        image = pygame.image.load(IMG_DIR).convert_alpha()
        CARD_WIDTH, CARD_HEIGHT = 50, 80
        return pygame.transform.scale(image, (CARD_WIDTH, CARD_HEIGHT))
    
    def get_value(self, game_mode: Mode) -> int:
        """Vrátí číselnou hodnotu karty na základě herního módu."""
        if game_mode == Mode.HRA:
            return self.rank.value
        
        if game_mode in (Mode.BETL, Mode.DURCH):
            # Pro betl a durch je deset pod devítkou
            # Proto musíte předefinovat pořadí karet.
            betl_order = {
                CardRanks.A: 8,
                CardRanks.K: 7,
                CardRanks.Q: 6,
                CardRanks.J: 5,
                CardRanks.X: 4,
                CardRanks.IX: 3,
                CardRanks.VIII: 2,
                CardRanks.VII: 1
            }
            return betl_order[self.rank]
            
        raise ValueError("Neplatný herní mód")

def card_mappping(input_str: str) -> Card:
    suit_str, rank_str = input_str.split(" ")
    
    if suit_str not in SUIT_LIST or rank_str not in RANK_LIST:
        rank_str, suit_str = input_str.split(" ")
        
    if suit_str not in SUIT_LIST or rank_str not in RANK_LIST:
        raise ValueError
    
    # Vytvoření a vrácení nové instance Card
    return Card(RANK_MAP[rank_str], SUITE_MAP[suit_str])

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
        """Metoda vytvoří standardní balíček 32 karet."""
        return [Card(rank, suit) for suit in CardSuits for rank in CardRanks]
        
    def shuffle(self) -> list:
        """Metoda náhodně zamíchá balíček karet."""
        deck = self.initialize_deck() # Zajistí, že balíček je kompletní
        random.shuffle(deck)
        self.card_index = 0 # Resetuje index po zamíchání
        return deck

    def deal_card(self) -> Card|None:
        """Vrátí další kartu z balíčku a posune index, nebo None pokud v balíčku už žádná karta není."""
        if self.has_next_card():
            card = self.cards[self.card_index]
            self.card_index += 1
            return card
        return None

    def has_next_card(self) -> bool:
        """Zkontroluje, zda v balíčku zbývají karty."""
        return self.card_index < len(self.cards)