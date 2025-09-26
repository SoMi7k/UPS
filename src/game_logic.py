from player import Player
from cards import Card, CardSuits, Mode

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
        