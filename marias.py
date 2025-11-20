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
        CARD_WIDTH, CARD_HEIGHT = 70, 100
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

class Player: 
    def __init__(self, number: int): 
        self.number = number 
        self.hand = Hand()
        
    def remove_hand(self):
        self.hand.remove_hand()
    
    def add_card(self, card: Card): 
        """Adds card to player from deck.""" 
        self.hand.add_card(card) 
        
    def add_won_card(self, card: Card): 
        """Moves won card into player's winning cards pack.""" 
        self.hand.add_won_card(card) 
        
    def has_card_in_hand(self) -> bool: 
        """Zkontroluje jestli má hráč ještě čím hrát.""" 
        return True if self.hand.cards else False 
    
    def has_won(self): 
        """Vyhrál hráč štych?""" 
        return True if self.hand.won_cards else False 
    
    def choose_card(self) -> Card: 
        """Allows the player to select and play a card with full validation.""" 
        # Inicializace proměnné played_card = None # Opakování smyčky, dokud není platná karta v ruce 
        while True: 
            played_card_input = input("Kterou kartu chcete zahrát?\n> ") 
            try: # Pokusí se namapovat vstup na objekt karty 
                played_card = card_mappping(played_card_input) 
                # Zkontroluje, zda je karta v ruce 
                if self.hand.find_card_in_hand(played_card): 
                    print(f"Zahrál si {played_card}") 
                    break 
                # Pokud je vše v pořádku, ukončí se smyčka 
                else: print("Karta není ve tvém balíčku karet! Zkus to znovu.") 
            except ValueError: # Pokud dojde k chybě při mapování (neplatný formát), 
                # vytiskne se chybová zpráva 
                print("Zadal jsi špatný kód karty! Zkus to znovu.") 
        return played_card 
    
    def play(self, 
             trick_suit: CardSuits|None, 
             trumph: CardSuits,
             played_card: Card,
             played_cards: list[Card], 
             mode: Mode) -> Card:
        """Ask player for a card then remove from the hand and returns it"""
        self.hand.remove_card(played_card)
        if trick_suit:
            while not self.check_played_card(trick_suit, trumph, played_card, played_cards, mode):
                self.hand.add_card(played_card)
                played_card = self.choose_card() 
                self.hand.remove_card(played_card)
        return played_card 
    
    def pick_cards(self, count: int):
        """Returns a specific amount of cards."""
        cards = []
        if count == "all":
            count = len(self.hand.cards)
        for i in range(count):
            if i > len(self.hand.cards):
                break
            cards.append(self.hand.cards[i])
        return cards
    
    def calculate_hand(self, mode: Mode) -> int:
        self.hand.calculate_hand(mode)
        return self.hand.points
    
    def check_played_card(self, trick_suit: CardSuits, trumph: CardSuits,
                        played_card: Card, played_cards: list[Card], mode: Mode) -> bool:
        """
        Ověří, zda hráč zahrál kartu podle pravidel mariáše.
        
        Args:
            trick_suit (CardSuits): barva první zahrané karty ve štychu
            trumph (CardSuits): trumfová barva
            played_card (Card): karta, kterou hráč chce zahrát
            played_cards (list[Card]): karty, které už byly zahrány ve štychu
            mode (Mode): aktuální mód hry

        Returns:
            bool: True = správně zahraná karta, False = porušení pravidel
        """
        played_cards = [card for card in played_cards if card is not None]

        # --- 1. má hráč stejnou barvu jako je povinná? ---
        if self.hand.find_card_by_suit(trick_suit):
            if played_card.suit != trick_suit:
                print("Musíš zahrát stejnou barvu!")
                return False  # musí dodržet barvu
            # --- 2. pokud už padla vyšší karta v barvě, musí ji přebít, pokud může ---
            highest_in_suit = max(
                [c for c in played_cards if c.suit == trick_suit],
                key=lambda c: c.get_value(mode),
                default=None
            )
            if highest_in_suit and played_card.get_value(mode) < highest_in_suit.get_value(mode):
                # hráč má vyšší kartu v barvě?
                if any(c.suit == trick_suit and c.get_value(mode) > highest_in_suit.get_value(mode)
                    for c in self.hand.cards):
                    print("Musíš zahrát vyšší kartu!")
                    return False
            return True

        # --- 3. hráč nemá barvu, ale má trumf ---
        if self.hand.find_card_by_suit(trumph):
            if played_card.suit != trumph:
                print("Musíš zahrát trumf!")
                return False
            # musí přebít trumf, pokud je už ve štychu
            highest_trump = max(
                [c for c in played_cards if c.suit == trumph],
                key=lambda c: c.get_value(mode),
                default=None
            )
            if highest_trump and played_card.get_value(mode) < highest_trump.get_value(mode):
                if any(c.suit == trumph and c.get_value(mode) > highest_trump.get_value(mode)
                    for c in self.hand.cards):
                    print("Musíš zahrát vyšší kartu!")
                    return False
            return True

        # --- 4. nemá ani barvu, ani trumf → může zahrát cokoli ---
        return True
    
    def sort_hand(self, mode=Mode.BETL):
        """Seřadí karty v ruce podle barvy a hodnoty."""
        self.hand.sort(mode)
            
    def __str__(self):
        return f"Player #{self.number}: {', '.join(str(card) for card in self.hand.cards)}"

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

class GameLogic:
    def __init__(self, num_players: int):
        self.trumph: CardSuits
        self.talon: list[Card] = []
        self.num_players: int = num_players
        self.mode: Mode|None = None
        
    def move_to_talon(self, card: Card, player: Player):
        """Přesune karty do talonu."""
        player.hand.remove_card(card)
        self.talon.append(card)
    
    def move_from_talon(self, player: Player):
        """V případě betlu, nebo durchu se převedou karty s talonu hráči, který chce tyto hru hrát."""
        for card in self.talon:
            player.add_card(card)
        
        self.talon = []
    
    def find_trumph(self, cards: list[Card], decision_list: list[bool]) -> list[bool]:
        """Vrátí pole, kde True znamená trumf a False opak."""
        for i, card in enumerate(cards):
            if card.suit != self.trumph:
                decision_list[i] = False
        
        return decision_list
    
    def same_suit_case(self, cards: list[Card]) -> int:
        """Vrátí index výherce štychu v případě, že jsou trumfy všechny karty."""
        winner_index = 0
        for i in range(1, len(cards)):
            # Porovnává aktuální kartu s kartou, kterou považujeme za vítěze
            if cards[i].get_value(self.mode) > cards[winner_index].get_value(self.mode):
                winner_index = i
                
        return winner_index
    
    def not_trumph_case(self, cards: list[Card], trick_suit: CardSuits) -> int:
        """Vrátí index výherce štychu v případě, že nikdo nezahrál trumf."""
        winner_index = 0
        for i in range(1, len(cards)):
            if cards[i].suit == trick_suit and cards[i].get_value(self.mode) > cards[winner_index].get_value(self.mode):
                winner_index = i
        return winner_index

    def trick_decision(self, cards: list[Card], start_player_index: int) -> tuple[int, Card]:
        """
        Vyhodnotí štych a vrátí index výherní karty.
        
        Args:
            cards (list[Card]): Pole karet zahraných v daném štychu
            start_player_index (int): Index začínajícího hráče

        Returns:
            int: Index vyhrané karty a výherní kartu
        """
        trick_suit = cards[start_player_index].suit
        decision_list = self.num_players * [True]
        
        if self.mode == Mode.HRA:
            decision_list = self.find_trumph(cards, decision_list)
            trumph_count = sum(decision_list)
            
            if trumph_count == 1:
                winning_index = decision_list.index(True)
                return winning_index, cards[winning_index]
            
            if trumph_count == 2:
                true_indices = []
                for index, value in enumerate(decision_list):
                    if value:
                        true_indices.append(index)

                if cards[true_indices[0]].get_value(self.mode) > cards[true_indices[1]].get_value(self.mode):
                    return true_indices[0], cards[true_indices[0]]
                else:
                    return true_indices[1], cards[true_indices[1]]
            
            if trumph_count == 3:
                winning_index = self.same_suit_case(cards)
                return winning_index, cards[winning_index]
            
        for i, card in enumerate(cards):
            if i == 0:
                continue
            if card.suit != trick_suit:
                decision_list[i] = False
    
        winning_index = self.not_trumph_case(cards, trick_suit)
        return winning_index, cards[winning_index]

TRUMPH_CARDS = 7 
class Hand: 
    def __init__(self): 
        self.cards = [] 
        self.won_cards = [] 
        self.points = 0 
    
    def check_right_input(self, card_input: str): 
        try: 
            card_mappping(card_input) 
            return False 
        except ValueError: 
            return False
        
    def sort(self, game_mode: Mode):
        """Seřadí karty v ruce podle barvy a hodnoty."""
        suit_order = {
            CardSuits.SRDCE: 0,
            CardSuits.KULE: 1,
            CardSuits.ZALUDY: 2,
            CardSuits.LISTY: 3
        }
        self.cards.sort(key=lambda c: (suit_order[c.suit], c.get_value(game_mode)), reverse=True)
        
    def find_card_in_hand(self, card_input: Card) -> bool: 
        for card in self.cards: 
            if card_input.__eq__(card): 
                return True 
        return False 
        
    def find_card_by_rank(self, rank: CardRanks, mode: Mode) -> bool: 
        for card in self.cards: 
            if card.get_value(mode) == rank.value: 
                return True 
        return False 
        
    def find_card_by_suit(self, suit: CardSuits) -> bool: 
        for card in self.cards: 
            if card.suit == suit: 
             return True 
        return False 
    
    def get_hand(self, mode: Mode):
        return self.sort(mode)
    
    def remove_card(self, card_to_remove: Card) -> None: 
        self.cards.remove(card_to_remove) 
        
    def add_card(self, card_to_add: Card) -> None: 
        self.cards.append(card_to_add) 
    
    def add_won_card(self, card: Card) -> None: 
        self.won_cards.append(card) 
        
    def calculate_hand(self, mode: Mode): 
        for card in self.won_cards: 
            if card.get_value(mode) == 7 or card.get_value(mode) == 8: 
                self.points += 10
                
    def remove_hand(self):
        self.cards = []

class GameLogic:
    def __init__(self, num_players: int):
        self.trumph: CardSuits
        self.talon: list[Card] = []
        self.num_players: int = num_players
        self.mode: Mode|None = None
        
    def move_to_talon(self, card: Card, player: Player):
        """Přesune karty do talonu."""
        player.hand.remove_card(card)
        self.talon.append(card)
    
    def move_from_talon(self, player: Player):
        """V případě betlu, nebo durchu se převedou karty s talonu hráči, který chce tyto hru hrát."""
        for card in self.talon:
            player.add_card(card)
        
        self.talon = []
    
    def find_trumph(self, cards: list[Card], decision_list: list[bool]) -> list[bool]:
        """Vrátí pole, kde True znamená trumf a False opak."""
        for i, card in enumerate(cards):
            if card.suit != self.trumph:
                decision_list[i] = False
        
        return decision_list
    
    def same_suit_case(self, cards: list[Card]) -> int:
        """Vrátí index výherce štychu v případě, že jsou trumfy všechny karty."""
        winner_index = 0
        for i in range(1, len(cards)):
            # Porovnává aktuální kartu s kartou, kterou považujeme za vítěze
            if cards[i].get_value(self.mode) > cards[winner_index].get_value(self.mode):
                winner_index = i
                
        return winner_index
    
    def not_trumph_case(self, cards: list[Card], trick_suit: CardSuits) -> int:
        """Vrátí index výherce štychu v případě, že nikdo nezahrál trumf."""
        winner_index = 0
        for i in range(1, len(cards)):
            if cards[i].suit == trick_suit and cards[i].get_value(self.mode) > cards[winner_index].get_value(self.mode):
                winner_index = i
        return winner_index

    def trick_decision(self, cards: list[Card], start_player_index: int) -> tuple[int, Card]:
        """
        Vyhodnotí štych a vrátí index výherní karty.
        
        Args:
            cards (list[Card]): Pole karet zahraných v daném štychu
            start_player_index (int): Index začínajícího hráče

        Returns:
            int: Index vyhrané karty a výherní kartu
        """
        trick_suit = cards[start_player_index].suit
        decision_list = self.num_players * [True]
        
        if self.mode == Mode.HRA:
            decision_list = self.find_trumph(cards, decision_list)
            trumph_count = sum(decision_list)
            
            if trumph_count == 1:
                winning_index = decision_list.index(True)
                return winning_index, cards[winning_index]
            
            if trumph_count == 2:
                true_indices = []
                for index, value in enumerate(decision_list):
                    if value:
                        true_indices.append(index)

                if cards[true_indices[0]].get_value(self.mode) > cards[true_indices[1]].get_value(self.mode):
                    return true_indices[0], cards[true_indices[0]]
                else:
                    return true_indices[1], cards[true_indices[1]]
            
            if trumph_count == 3:
                winning_index = self.same_suit_case(cards)
                return winning_index, cards[winning_index]
            
        for i, card in enumerate(cards):
            if i == 0:
                continue
            if card.suit != trick_suit:
                decision_list[i] = False
    
        winning_index = self.not_trumph_case(cards, trick_suit)
        return winning_index, cards[winning_index]

ROZDAVANI_KARET = [7, 5, 5, 5]

# Hlavní třída Game
class Game:
    """
    Třída, ve které se nachází všechny prvky potřebné pro hru Blackjack.
    Obsahuje metody pro funkci hry.
    """
    def __init__(self, num_players=3):
        self.num_players = num_players
        self.players = [Player(i) for i in range(num_players)]
        self.licitator = self.players[0]
        self.deck = Deck()
        self.game_logic = GameLogic(num_players)
        self.played_cards = num_players * [None]
        self.state = State.ROZDÁNÍ_KARET
        self.higher_game = False
        self.higher_player = None
        self.starting_player_index = self.licitator.number
        self.active_player = self.licitator
        self.trick_suit = None
        self.trick_winner = None
        self.trick_cards = None
        self.waiting_for_trick_end = False
        self.result_str = ""
        
    def deal_cards(self):
        """Rozdá karty hráčům a licitátorovi."""
        for _ in range(ROZDAVANI_KARET[0]):
            self.licitator.add_card(self.deck.deal_card())
        for _ in range(ROZDAVANI_KARET[1]):
            for player in self.players:
                if player != self.licitator:
                    player.add_card(self.deck.deal_card())
        for _ in range(ROZDAVANI_KARET[2]):
            self.licitator.add_card(self.deck.deal_card())
        for _ in range(ROZDAVANI_KARET[3]):
            for player in self.players:
                if player != self.licitator:
                    player.add_card(self.deck.deal_card())
        for player in self.players:
            player.sort_hand()
            
    def reset_game(self):
        """Resetuje hru na začátek: obnoví ruce a stav hráčů."""
        for player in self.players:
            player.remove_hand()
        self.deck.shuffle()
        
    def next_player(self):
        actual_player_index = self.active_player.number
        self.active_player = self.players[(actual_player_index + 1) % self.num_players]          
    
    def game_state_1(self, card: Card):
        self.game_logic.trumph = card.suit
        self.state = State.LICITACE_TALON
        
    def game_state_2(self, card: Card):
        self.game_logic.move_to_talon(card, self.active_player)
        
        if len(self.game_logic.talon) == 2:
            if self.higher_game and self.game_logic.mode == Mode.BETL and self.higher_player.number != self.num_players - 1:
                self.talon = 0
                self.state = State.LICITACE_DOBRY_SPATNY
                self.next_player()
                return
            if self.higher_game and self.game_logic.mode == Mode.BETL:
                self.state = State.BETL
                return
            if self.higher_game and self.game_logic.mode == Mode.DURCH:
                self.state = State.DURCH
                return
            self.state = State.LICITACE_HRA
            self.talon = 0
        
    def game_state_3(self, label: str):
        if self.game_logic.mode:
            return
        if label == Mode.HRA.name:
            self.game_logic.mode = Mode.HRA
        elif label == Mode.BETL.name:
            self.higher(self.active_player, Mode.BETL)
        else:
            self.higher(self.active_player, Mode.DURCH)
            self.state = State.DURCH
            return
        
        self.state = State.LICITACE_DOBRY_SPATNY
        self.next_player()
        
    def game_state_4(self, label: str):
        if label == "Špatný" and not self.higher_game:
            self.higher_game = True
            self.higher_player = self.active_player
            self.state = State.LICITACE_BETL_DURCH
            return
        #if label == "Špatný" and not self.higher_game and self.game_logic.mode: # TODO durch po betlu   
        if label == "Špatný" and self.higher_game:
            self.higher_game = True
            self.higher(self.active_player, Mode.DURCH)
            return
        if label == "Dobrý" and self.higher_game:
            self.state = State.BETL
            self.active_player = self.players[self.higher_player.number] 
            return
        if label == "Dobrý" and self.active_player.number == self.num_players - 1:
            self.choose_mode_state()
            
        self.next_player()
        
    def game_state_5(self, label: str):
        if label == "DURCH":
            self.higher(self.active_player, Mode.DURCH)
            return
        if label == "BETL" and self.higher_player.number != self.num_players - 1:
            self.higher(self.active_player, Mode.BETL)
        else:
            self.higher(self.active_player, Mode.BETL)
            self.higher_player = self.active_player
            
    def choose_mode_state(self):
        """Hráč vybýrá daný mód"""
        match self.game_logic.mode:
            case Mode.HRA:
                self.state = State.HRA
                self.game_logic.mode = Mode.HRA
            case Mode.BETL:
                self.state = State.BETL
                self.game_logic.mode = Mode.BETL
            case Mode.DURCH:
                self.state = State.DURCH
                self.game_logic.mode = Mode.DURCH
            case _:
                self.state = State.HRA
                self.game_logic.mode = Mode.HRA
          
    def higher(self, player: Player, mode: Mode):
        self.game_logic.mode = mode
        self.game_logic.trumph = None
        self.higher_player = player
        if player != self.licitator:
            self.licitator = player
            self.game_logic.move_from_talon(player)
            self.state = State.LICITACE_TALON
            
    def check_stych(self) -> bool:
        if self.waiting_for_trick_end:
            for won_card in self.trick_cards:
                self.players[self.trick_winner].add_won_card(won_card)
            self.played_cards = self.num_players * [None]
            self.trick_suit = None
            self.starting_player_index = self.trick_winner
            self.trick_winner = None
            self.trick_cards = None
            self.waiting_for_trick_end = False
            self.active_player = self.players[self.starting_player_index]
            return True
        return False
        
    def game_state_6(self, card: Card) -> bool:
        if self.trick_suit:
            if not self.active_player.check_played_card(
                self.trick_suit, self.game_logic.trumph, card, self.played_cards, self.game_logic.mode
            ):
                return False
        else:
            self.trick_suit = card.suit

        self.played_cards[self.active_player.number] = card
        self.active_player.hand.remove_card(card)

        # Štych kompletní?
        if all(c is not None for c in self.played_cards):
            winner_index, _ = self.game_logic.trick_decision(self.played_cards, self.starting_player_index)
            self.trick_winner = winner_index
            self.trick_cards = self.played_cards[:]  # snapshot karet pro vykreslení
            self.waiting_for_trick_end = True

        if all(player.has_card_in_hand() is False for player in self.players):
            self.result_str = self.game_result(None)
            self.state = State.END

        self.next_player()
        return True

    def game_state_7(self, card: Card):    
        if self.trick_suit:
            if not self.active_player.check_played_card(self.trick_suit, self.game_logic.trumph, card, self.played_cards, self.game_logic.mode):
                return False
        else:
            self.trick_suit = card.suit

        self.played_cards[self.active_player.number] = card
        self.active_player.hand.remove_card(card)
        
        if all(c is not None for c in self.played_cards):
            winner_index, _ = self.game_logic.trick_decision(self.played_cards, self.starting_player_index)
            self.trick_winner = winner_index
            self.trick_cards = self.played_cards[:]  # snapshot karet pro vykreslení
            self.waiting_for_trick_end = True
            
            if self.game_logic.mode == Mode.BETL:
                if winner_index == self.licitator.number:
                    self.result_str = self.game_result(False)
                    self.state = State.END
                    return True
            else:          
                if winner_index != self.licitator.number:
                    self.result_str = self.game_result(False)
                    self.state = State.END
                    return True
            
            for won_card in self.played_cards:
                self.players[winner_index].add_won_card(won_card)
                
        if all(player.has_card_in_hand() is False for player in self.players):
            self.result_str = self.game_result(True)
            self.state = State.END
        
        self.next_player()
        return True
    
    def game_state_8(self, label: str) -> bool:
        if label == "ANO":
            return True
        else:
            return False

    def game_result(self, res: None|bool) -> str:
        if res is not None:
            if res:
                return f"Vyhrál Hráč #{self.licitator.number} jako licitátor!"
            else:
                return f"Prohrál Hráč #{self.licitator.number} jako licitátor!"
        
        players_points = 0
        licitator_points = self.licitator.calculate_hand(self.game_logic.mode)
        for player in self.players:
            if player != self.licitator:
                players_points += player.calculate_hand(self.game_logic.mode)
                
        if licitator_points > players_points:
            return f"Vyhrál Hráč #{self.licitator.number} jako licitátor v poměru {licitator_points} : {players_points}"
        elif licitator_points == players_points:
            return f"Remíza  v poměru Hráč #{self.licitator.number} jako licitátor {licitator_points} : Hráči proti {players_points}"
        else:   
            return f"Prohrál Hráč #{self.licitator.number} jako licitátor v poměru {licitator_points} : {players_points}"
        
import pygame
import sys

#root = tk.Tk()
WIDTH = 1000 #root.winfo_screenwidth()
HEIGHT = 600 #root.winfo_screenheight()
#root.destroy()
CARD_WIDTH, CARD_HEIGHT = 70, 100
IMG_DIR = "C:\\Users\\Lenka Jelinková\\Desktop\\UPS\\images\\wooden_table.jpg"

class GUI:
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
        self.draw_text("Licitátor vybírá trumf.", None, 20)
        self.draw_cards(self.game.active_player.pick_cards(7))
         
    def draw_state_2(self):
        self.draw_text("Licitátor odhazuje karty do talonu.", None, 20)
        self.draw_cards(self.game.active_player.pick_cards("all"))
        
    def draw_state_3(self):
        self.draw_text("Licitátor vybírá hru.", None, 20)
        self.draw_cards(self.game.active_player.pick_cards("all"))
        self.draw_selection_buttons([mode.name for mode in Mode])
        
    def draw_state_4(self):
        mode_name = self.game.game_logic.mode.name
        trumph = self.game.game_logic.trumph
        self.draw_text(f"{mode_name} {trumph.name if trumph else ""}")
        self.draw_text(f"Hráč #{self.game.active_player.number}", None, 20)
        self.draw_cards(self.game.active_player.pick_cards("all"))
        self.draw_selection_buttons(["Dobrý", "Špatný"])

    def draw_state_5(self):
        self.draw_text(f"Hráč #{self.game.active_player.number}", None, 20)
        self.draw_cards(self.game.active_player.pick_cards("all"))
        self.draw_selection_buttons(["BETL", "DURCH"])
    
    def draw_state_6(self):
        self.draw_text(f"Hráč #{self.game.active_player.number}", None, 20)
        self.draw_selection_buttons(["Špatný"])
        self.draw_cards(self.game.active_player.pick_cards("all"))
        
    def draw_state_7(self):
        self.draw_text(f"Hráč #{self.game.active_player.number}", None, 20)
        mode_name = self.game.game_logic.mode.name
        trumph = self.game.game_logic.trumph
        self.draw_text(f"{mode_name} {trumph.name if trumph else ""}", None, 60)
        self.draw_played_cards(self.game.played_cards)
        self.draw_cards(self.game.active_player.pick_cards("all"))
        
    def draw_state_8(self):
        self.draw_text(self.game.result_str, None, 20)
        self.draw_text("Resetovat hru?", None, 100)
        self.draw_selection_buttons(["ANO", "NE"],)
        
    def draw(self):
        """Vykreslí daný stav hry."""
        overlay = pygame.Surface((WIDTH, HEIGHT), pygame.SRCALPHA)  
        overlay.fill((255, 255, 255, 30))  # RGBA → 50 = průhlednost
        self.screen.blit(overlay, (0, 0))
        self.screen.blit(self.background, (0, 0))
        
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
    