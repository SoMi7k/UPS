from card import Card, card_mappping, State
from player import Hand, Player
from game import Game

class msgHandler:
    def __init__(self,):
        self.game: Game
    
    def set_game(self, game: Game):
        self.game = game
    
    def card_wrapper(self, card: Card) -> str:
        rank = card.rank
        suit = card.suit        
        return f"{rank} {suit}"
    
    def card_reader(self, card: str) -> Card:
        new_card = card_mappping(card)
        return new_card
    
    def player_reader(self, player_data: dict):
        number = player_data["number"]
        nickname = player_data["nickname"]
        hand = Hand()
        for card in player_data["hand"]:
            new_card = card_mappping(card)
            hand.add_card(new_card)
        player = Player(number, nickname, hand)

        self.game.add_player(player)
    
    def state_reader(self, data: dict):
        # Mapování čísla stavu na State enum
        if data["change_state"]:
            state_num = int(data["state"])
            try:
                self.game.state  = State(state_num)
            except ValueError:
                print("Stav hry se špatně načetl")
                self.game.state  = State.CHYBOVÝ_STAV
            
        if data["gameStarted"]:
            self.game.init_mode(data["mode"])
            self.game.init_trump(data["trump"])
            if data["isPlayedCards"]:
                if data["change_trick"]:
                    self.game.change_trick = True
                for card in data["played_cards"]:
                    new_card = self.card_reader(card)
                    self.game.add_played_card(new_card)
                    
    def status_reader(self, data: dict):
        pass
        
    def game_start_reader(self, data: dict):
        self.player_reader(data["client"])
        self.game.init_players(data)
        
    def game_result_reader(self, data: dict):
        score = data["gameResult"]
        self.game.evaluate_result(score)
            
if __name__ == "__main__":       
    data = {
        "client": {
            "number": "2",
            "nickname": "Honza",
            "hand": {
                "5 ♠",
                "5 ♠",
                "5 ♠",
                "5 ♠",
                "5 ♠",
            }
        },
        "players": [
            "0-testplayer",
            "1-testplayer"
        ],
        "licitator": "0",
        "active_player": "0"
    }


"""
f

player_card = {
    "card": "k ♦",
    "player": "TestPlayer1" 
}

state_hra = {
    "state": "3",
    "change_trick": "1",
    "change_state": "1"
}

status = {
    "envil": "1",
    "horfas": "1",
    "olyas58": "0",
    "Server": "1"
}
"""