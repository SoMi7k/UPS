import pytest

import sys
import os

# Add the parent directory to the Python path
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))

from src.game_logic import GameLogic
from src.cards import Card, CardRanks, CardSuits, Mode
from src.player import Player

# Helper function to create a GameLogic instance for testing
@pytest.fixture
def game_logic_instance():
    # You can customize this fixture if different tests need different setups
    game_logic = GameLogic(num_players=3)
    game_logic.mode = Mode.HRA
    return game_logic

# Helper function to create a player
@pytest.fixture
def player_instance():
    return Player(number=1)

def test_trick_decision_one_trumph(game_logic_instance: GameLogic, player_instance: Player):
    """Test case where only one player plays a trumph card."""
    # Setup the trumph suit
    game_logic_instance.trumph = CardSuits.SRDCE # ♥

    # Define the cards played in the trick
    # Player 1 (start player): Kule 10
    # Player 2: Listy Spodek
    # Player 3 (plays trumph): Srdce 8
    cards = [
        Card(CardRanks.DESET, CardSuits.KULE),
        Card(CardRanks.SPODEK, CardSuits.LISTY),
        Card(CardRanks.OSM, CardSuits.SRDCE) 
    ]
    
    # Act
    winner_index, _ = game_logic_instance.trick_decision(cards, start_player_index=0)
    
    # Assert
    # The winner should be the player who played the single trumph, which is at index 2.
    assert winner_index == 2

def test_trick_decision_two_trumph(game_logic_instance: GameLogic, player_instance: Player):
    """Test case where two players play a trumph card."""
    # Setup the trumph suit
    game_logic_instance.trumph = CardSuits.SRDCE # ♥

    # Define the cards played in the trick
    # Player 1 (start player): Kule 10
    # Player 2: Srdce Kral (Trumph)
    # Player 3: Srdce Svrsek (Trumph)
    cards = [
        Card(CardRanks.DESET, CardSuits.KULE),
        Card(CardRanks.KRAL, CardSuits.SRDCE),
        Card(CardRanks.SVRSEK, CardSuits.SRDCE)
    ]
    
    # Act
    winner_index, _ = game_logic_instance.trick_decision(cards, start_player_index=0)
    
    # Assert
    # The winner should be the player who played the single trumph, which is at index 2.
    assert winner_index == 1
    
def test_trick_decision_no_trumphs(game_logic_instance: GameLogic, player_instance: Player):
    """Test case where no players play a trumph card."""
    # Setup the trumph suit
    game_logic_instance.trumph = CardSuits.SRDCE # ♥
    
    # Define the cards played in the trick
    # Player 1 (start player): Kule 10
    # Player 2: Kule Kral
    # Player 3: Listy Eso
    cards = [
        Card(CardRanks.OSM, CardSuits.KULE),
        Card(CardRanks.KRAL, CardSuits.KULE),
        Card(CardRanks.ESO, CardSuits.LISTY)
    ]
    
    # Act
    winner_index, _ = game_logic_instance.trick_decision(cards, start_player_index=0)
    
    # Assert
    # The winner should be the player who played the single trumph, which is at index 2.
    assert winner_index == 1
    
def test_trick_decision_three_trumphs(game_logic_instance: GameLogic, player_instance: Player):
    """Test case where no players play a trumph card."""
    # Setup the trumph suit
    game_logic_instance.trumph = CardSuits.SRDCE # ♥
    
    # Define the cards played in the trick
    # Player 1 (start player): Kule 10
    # Player 2: Kule Kral
    # Player 3: Listy Eso
    cards = [
        Card(CardRanks.ESO, CardSuits.SRDCE),
        Card(CardRanks.KRAL, CardSuits.SRDCE),
        Card(CardRanks.OSM, CardSuits.SRDCE)
    ]
    
    # Act
    winner_index, _ = game_logic_instance.trick_decision(cards, start_player_index=0)
    
    # Assert
    # The winner should be the player who played the single trumph, which is at index 2.
    assert winner_index == 0