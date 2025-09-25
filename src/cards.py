from enum import Enum
import random

# Lists for mapping
SUIT_LIST = ['♥', '♦', '♣', '♠']
RANK_LIST = ['a', 'k', 'q', 'j', '10', '9', '8', '7']

class Mode(Enum):
    HRA = 1
    SEDMA = 2
    BETL = 4
    DURCH = 5
    
class CardRanks(Enum):
    # Příklad hodnot, nahraďte je skutečnými hodnotami karet
    ESO = 8
    DESET = 7
    KRAL = 6
    SVRSEK = 5
    SPODEK = 4
    DEVET = 3
    OSM = 2
    SEDM = 1
    

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
    'a': CardRanks.ESO,
    'k': CardRanks.KRAL,
    'q': CardRanks.SVRSEK,
    'j': CardRanks.SPODEK,
    '10': CardRanks.DESET,
    '9': CardRanks.DEVET,
    '8': CardRanks.OSM,
    '7': CardRanks.SEDM
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
        """
        Porovná mezi sebou 2 karty.
        """
        if isinstance(other, Card):
            return self.rank == other.rank and self.suit == other.suit
        return False

    def __str__(self):
        # Mapování barev na ANSI escape kódy
        color_map = {
            CardSuits.SRDCE: '\033[35m', # Světle červená
            CardSuits.KULE: '\033[31m',  # Světle červená
            CardSuits.ZALUDY: '\033[33m', # Žlutá
            CardSuits.LISTY: '\033[32m'   # Šedá/Černá
        }
        
        reset_color = '\033[0m'
        
        # Získání barvy pro danou barvu karty
        color_code = color_map.get(self.suit, reset_color)
        
        return f"{color_code}{self.rank.name} {self.suit.value}{reset_color}"

    def __repr__(self):
        """
        Metoda pro formální reprezentaci objektu, užitečná pro debugování.
        """
        return f"Card(rank={self.rank}, suit={self.suit})"
    
    def get_value(self, game_mode: Mode) -> int:
        """
        Vrátí číselnou hodnotu karty na základě herního módu.
        """
        if game_mode in (Mode.HRA, Mode.SEDMA):
            return self.rank.value
        
        if game_mode in (Mode.BETL, Mode.DURCH):
            # Pro betl a durch je deset pod devítkou
            # Proto musíte předefinovat pořadí karet.
            betl_order = {
                CardRanks.ESO: 8,
                CardRanks.KRAL: 7,
                CardRanks.SVRSEK: 6,
                CardRanks.SPODEK: 5,
                CardRanks.DESET: 4,
                CardRanks.DEVET: 3,
                CardRanks.OSM: 2,
                CardRanks.SEDM: 1
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
        self.cards = self.initialize_deck()
        self.card_index = 0

    def initialize_deck(self) -> list:
        """
        Metoda vytvoří standardní balíček 32 karet.
        """
        return [Card(rank, suit) for suit in CardSuits for rank in CardRanks]
        
    def shuffle(self) -> None:
        """
        Metoda náhodně zamíchá balíček karet.
        """
        self.initialize_deck() # Zajistí, že balíček je kompletní
        random.shuffle(self.cards)
        self.card_index = 0 # Resetuje index po zamíchání

    def deal_card(self) -> Card|None:
        """
        Vrátí další kartu z balíčku a posune index.
        Vrátí None, pokud v balíčku už žádná karta není.
        """
        if self.has_next_card():
            card = self.cards[self.card_index]
            self.card_index += 1
            return card
        return None

    def has_next_card(self) -> bool:
        """
        Zkontroluje, zda v balíčku zbývají karty.
        """
        return self.card_index < len(self.cards)