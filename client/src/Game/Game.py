from .Card import Card, Mode, CardSuits
from .Player import Player
from enum import Enum

class State(Enum):
    CHYBOVÝ_STAV = 0
    LICITACE_TRUMF = 1
    LICITACE_TALON = 2
    LICITACE_HRA = 3
    LICITACE_DOBRY_SPATNY = 4
    LICITACE_BETL_DURCH = 5
    HRA = 6
    BETL = 7
    DURCH = 7
    END = 8

# Hlavní třída Game
class Game:
    """
    Třída, ve které se nachází všechny prvky potřebné pro hru Mariáš.
    Obsahuje metody pro funkci hry.
    """
    def __init__(self, numPlayers: int, clientNumber: int):
        self.players: list = numPlayers * [None]
        self.state = State.LICITACE_TRUMF
        self.played_cards: list[Card] = []
        self.active_player: bool = False
        self.licitator: Player = None
        self.trumph: CardSuits|None = None
        self.mode: Mode|None = None
        self.result: str = ""
        self.clientNumber: int = clientNumber
        self.change_trick: bool = False
    
    def add_player(self, player: Player):
        self.players[player.number] = player
    
    def trick_changer(self, data: dict):
        if data["trick_changed"]:
            self.played_cards = []
    
    def add_played_card(self, card: Card):
        if card not in self.played_cards:
            self.played_cards.append(card)
            
    def state_changer(self, data: dict):
        if data["state_changed"]:
            next_state_value = self.state.value + 1
            self.state = State(next_state_value)
                
    def is_active_player(self, active_player: int) -> bool:
        if self.clientNumber == active_player:
            return True
        else:
            return False
        
    def init_trump(self, trump: str):
        """Inicializuje trumf podle stringu ze serveru (např. 'SRDCE' → CardSuits.SRDCE)."""
        try:
            self.trumph = CardSuits[trump]  # Enum lookup podle názvu
            print(f"🔧 Nastaven trumf {self.trumph}")
        except KeyError:
            print(f"⚠️ Neznámý trumf '{trump}', nastavuji None.")
            self.trumph = None

    def init_mode(self, mode: int):
        """Inicializuje herní mód podle stringu ze serveru (např. 'BETL' → Mode.BETL)."""
        try:
            self.mode = Mode(mode)  # Převod ze stringu na Enum
            print(f"🔧 Nastaven mód {self.mode}")
        except KeyError:
            print(f"⚠️ Neznámý mód '{mode}', nastavuji default (HRA).")
            self.mode = Mode.HRA
    
    def init_players(self, game_data: list):
        # <PLAYER>|<cards>|<players>|<licitator>|<activePlayer>
        players = game_data[2][:-1].split(":")
        print(f"SER PLAYERS: {players}")
        for player in players:
            tokens = player.split("-")
            self.add_player(Player(int(tokens[0]), tokens[1], None))
            
        self.licitator = self.players[int(game_data[3])]
        self.active_player = self.is_active_player(int(game_data[4]))
        print(f"Já jsem hráč {self.clientNumber} a aktivní hráč je {int(game_data[4])} => {self.active_player}")
        
    def handle_trick(self):
        self.change_trick = False
        self.played_cards = []    
        
    def evaluate_result(self, score: tuple):
        first, second = score[0], score[1]
        score_str = f"{first}:{second}"
        
        if first == 1:
            if self.clientNumber == self.licitator.number:
                self.result = "Vyhrál jsi!"
            else:
                self.result = f"{self.licitator.nickname} vyhrál!"
        
        elif second == 1:
            if self.clientNumber == self.licitator.number:
                self.result = "Prohrál jsi!"
            else:
                self.result = f"{self.licitator.nickname} prohrál!"
        
        else:
            if first == second:
                self.result = f"Remíza v poměru {score_str}"
            elif first > second:
                if self.clientNumber == self.licitator.number:
                    self.result = f"Vyhrál jsi v poměru {score_str}!"
                else:
                    self.result = f"{self.licitator.nickname} vyhrál v poměru {score_str}!"
            else:
                if self.clientNumber == self.licitator.number:
                    self.result = f"Prohrál jsi v poměru {score_str}!"
                else:
                    self.result = f"{self.licitator.nickname} prohrál v poměru {score_str}!"