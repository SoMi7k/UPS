from enum import Enum
import random

# Lists for mapping
SUIT_LIST = ['♥', '♦', '♣', '♠']
RANK_LIST = ['A', 'K', 'Q', 'J', '10', '9', '8', '7']

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
        # Mapování barev na ANSI escape kódy
        """
        color_map = {
            CardSuits.SRDCE: '\033[35m', # Světle červená
            CardSuits.KULE: '\033[31m',  # Světle červená
            CardSuits.ZALUDY: '\033[33m', # Žlutá
            CardSuits.LISTY: '\033[32m'   # Šedá/Černá
        }
        
        reset_color = '\033[0m'
        
        # Získání barvy pro danou barvu karty
        color_code = color_map.get(self.suit, reset_color)
        """
        
        return f"{self.rank.name} {self.suit.value}"

    def __repr__(self):
        """Metoda pro formální reprezentaci objektu, užitečná pro debugování."""
        return f"Card(rank={self.rank}, suit={self.suit})"
    
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