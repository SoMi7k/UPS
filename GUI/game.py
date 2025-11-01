from card import Card, State, Mode, CardSuits, card_mappping
from GUI.player import Player

# Hlavní třída Game
class Game:
    """
    Třída, ve které se nachází všechny prvky potřebné pro hru Blackjack.
    Obsahuje metody pro funkci hry.
    """
    def __init__(self, numPlayers: int, clientNumber: int):
        self.players: list = numPlayers * [None]
        self.state = State.LICITACE_TRUMF
        self.played_cards: list[Card] = []
        self.active_player: bool
        self.licitator: Player
        self.trumph: CardSuits
        self.mode: Mode
        self.result: str = ""
        self.clientNumber = clientNumber
        
    def add_player(self, player: Player):
        self.players[player.number] = player
    
    def trick_changer(self, data: dict):
        if data["trick_changed"]:
            self.played_cards = []
    
    def add_played_card(self, data: dict):
        try:
            if isinstance(data, dict):
                card = card_mappping(data["card"])
                self.played_cards.append(card)
        except Exception as e:
            print(f"ERROR 0: {e}")
            
    def state_changer(self, data: dict):
        if data["state_changed"]:
            next_state_value = self.state.value + 1
            self.state = State(next_state_value)
                
    def is_active_player(self, active_player: int) -> bool:
        if self.clientNumber == active_player:
            return True
        else:
            return False
    
    def init_players(self, game_data: dict[str, str]):
        for player in game_data["players"]:
            tokens = player.split("-")
            self.add_player(Player(int(tokens[0]), tokens[1], None))
            
        self.licitator = self.players[int(game_data["licitator"])]
        self.active_player = self.is_active_player(int(game_data["licitator"]))
    
    
    