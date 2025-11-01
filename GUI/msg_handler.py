from card import Card, card_mappping, State
from player import Hand, Player
from GUI.game import Game

class msgHandler:
    def __init__(self):
        self.game: Game
    
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
        
    def reader_playerCard(self, data: dict):
        card = self.card_reader(data["card"])
        player = data["player"]
        
        self.game.add_played_card({ "card": card, "player": player })
    
    def state_reader(self, data: dict):
        result = {
            "current_state": None,
            "state_changed": bool(int(data.get("change_state", "0"))),
            "trick_changed": bool(int(data.get("change_trick", "0"))),
            "state_name": None
        }
        
        # Mapování čísla stavu na State enum
        state_num = int(data.get("state", "0"))
        try:
            result["current_state"] = State(state_num)
            result["state_name"] = result["current_state"].name
        except ValueError:
            result["current_state"] = None
            result["state_name"] = "UNKNOWN"
        
        return result
    
    def status_reader(self, data: dict):
        pass
    
    def game_start_reader(self, data: dict):
        self.player_reader(data["client"])
        self.game.init_players(data)
            
        

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