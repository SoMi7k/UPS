from src.cards import Card, CardRanks, CardSuits, Mode
from src.player import Player

# Helper pro vytvoření hráče s danými kartami
def make_player_with_cards(cards: list[Card]) -> Player:
    p = Player(0)
    p.hand.cards = cards
    return p


def test_must_follow_suit():
    # Hráč má srdce (trick_suit), ale zahraje žaludy -> špatně
    player = make_player_with_cards([
        Card(CardRanks.ESO, CardSuits.SRDCE),
        Card(CardRanks.DESET, CardSuits.ZALUDY)
    ])
    trick_suit = CardSuits.SRDCE
    trumph = CardSuits.KULE
    played = Card(CardRanks.DESET, CardSuits.ZALUDY)
    assert not player.check_played_card(trick_suit, trumph, played, [], Mode.HRA)


def test_follow_suit_correctly():
    # Hráč má srdce a zahraje srdce -> správně
    player = make_player_with_cards([
        Card(CardRanks.ESO, CardSuits.SRDCE),
        Card(CardRanks.DESET, CardSuits.ZALUDY)
    ])
    trick_suit = CardSuits.SRDCE
    trumph = CardSuits.KULE
    played = Card(CardRanks.ESO, CardSuits.SRDCE)
    assert player.check_played_card(trick_suit, trumph, played, [], Mode.HRA)


def test_must_overtrump_in_suit():
    # Ve štychu už padl král srdcí, hráč má eso srdcí -> musí ho přebít
    player = make_player_with_cards([
        Card(CardRanks.ESO, CardSuits.SRDCE),
        Card(CardRanks.DESET, CardSuits.ZALUDY)
    ])
    trick_suit = CardSuits.SRDCE
    trumph = CardSuits.KULE
    already_played = [Card(CardRanks.KRAL, CardSuits.SRDCE)]
    played = Card(CardRanks.DESET, CardSuits.ZALUDY)  # pokusí se utéct žaludy
    assert not player.check_played_card(trick_suit, trumph, played, already_played, Mode.HRA)


def test_trump_required_if_no_suit():
    # Trick vede žaludy, hráč nemá žaludy, ale má trumf -> musí zahrát trumf
    player = make_player_with_cards([
        Card(CardRanks.DESET, CardSuits.SRDCE),   # trumf
        Card(CardRanks.DESET, CardSuits.LISTY)
    ])
    trick_suit = CardSuits.ZALUDY
    trumph = CardSuits.SRDCE
    played = Card(CardRanks.DESET, CardSuits.LISTY)  # hraje listy místo trumfu
    assert not player.check_played_card(trick_suit, trumph, played, [], Mode.HRA)


def test_can_play_anything_if_no_suit_no_trump():
    # Trick vede žaludy, hráč nemá žaludy ani trumf -> může zahrát cokoli
    player = make_player_with_cards([
        Card(CardRanks.DESET, CardSuits.LISTY),
        Card(CardRanks.OSM, CardSuits.SRDCE)  # trumf je ♦, toto není trumf
    ])
    trick_suit = CardSuits.ZALUDY
    trumph = CardSuits.KULE  # ♦ jako trumf
    played = Card(CardRanks.DESET, CardSuits.LISTY)
    assert player.check_played_card(trick_suit, trumph, played, [], Mode.HRA)