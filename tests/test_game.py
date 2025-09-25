import sys
import os

# Add the parent directory to the Python path
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))

# test_marias.py
from src.cards import Card, CardRanks, CardSuits, Mode
from src.player import Player
from src.game import Game

def test_game_mode_hra():
    print("==== TEST MARIÁŠ: MOD HRA ====")

    # Inicializace hry se 3 hráči
    game = Game(num_players=3)

    # Rozdáme karty
    game.deal_cards()

    # Natvrdo nastavíme trumf a mód hry (aby se neptalo na input)
    game.game_logic.trumph = CardSuits.SRDCE
    game.game_logic.mode = Mode.HRA

    print("\n--- Rozdané karty ---")
    for player in game.players:
        print(player)

    # Hra (přepíšeme choose_card, aby hrál první kartu z ruky automaticky)
    for player in game.players:
        def auto_choose_card(self_player=player):
            return self_player.hand.cards[0]  # vždy vrátí první kartu
        player.choose_card = auto_choose_card.__get__(player, Player)

    print("\n--- Začíná hra ---")
    game.play()
    print("\n--- Konec hry ---")

def test_game_mode_betl_lost():
    print("==== TEST MARIÁŠ: MOD BETL ====")

    # Inicializace hry se 3 hráči
    game = Game(num_players=3)

    game.players[0].hand.cards = [
        Card(CardRanks.SEDM, CardSuits.SRDCE),
        Card(CardRanks.SEDM, CardSuits.KULE),
        Card(CardRanks.SEDM, CardSuits.LISTY),
        Card(CardRanks.SEDM, CardSuits.ZALUDY),
        Card(CardRanks.ESO, CardSuits.ZALUDY),
    ]
    game.players[1].hand.cards = [
        Card(CardRanks.DEVET, CardSuits.SRDCE),
        Card(CardRanks.KRAL, CardSuits.KULE),
        Card(CardRanks.DEVET, CardSuits.ZALUDY),
        Card(CardRanks.SVRSEK, CardSuits.ZALUDY),
        Card(CardRanks.DEVET, CardSuits.LISTY),
    ]
    game.players[2].hand.cards = [
        Card(CardRanks.OSM, CardSuits.SRDCE),
        Card(CardRanks.DESET, CardSuits.KULE),
        Card(CardRanks.OSM, CardSuits.LISTY),
        Card(CardRanks.DESET, CardSuits.ZALUDY),
        Card(CardRanks.DESET, CardSuits.LISTY),
    ]
    
    # Natvrdo nastavíme trumf a mód hry (aby se neptalo na input)
    game.game_logic.trumph = None
    game.game_logic.mode = Mode.BETL

    print("\n--- Rozdané karty ---")
    for player in game.players:
        print(player)

    # Hra (přepíšeme choose_card, aby hrál první kartu z ruky automaticky)
    for player in game.players:
        def auto_choose_card(self_player=player):
            return self_player.hand.cards[0]  # vždy vrátí první kartu
        player.choose_card = auto_choose_card.__get__(player, Player)

    print("\n--- Začíná hra ---")
    game.play()
    print("\n--- Konec hry ---")
    
    assert not game.licitator_won()
    
if __name__ == "__main__":
    test_game_mode_hra()
    test_game_mode_betl_lost()