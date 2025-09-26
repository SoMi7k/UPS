from player import Player
from cards import Card, Deck, Mode, State
from game_logic import GameLogic

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
        self.talon = 0
        self.higher_game = False
        self.higher_player = None
        self.trick_suit = None
        self.starting_player_index = self.licitator.number
        self.result_str = ""
        self.active_player = self.licitator
        
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
        self.talon += 1
        
        if self.talon == 2:
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
    
    def game_state_6(self, card: Card) -> bool:
        if self.trick_suit:
            if not self.active_player.check_played_card(self.trick_suit, self.game_logic.trumph, card, self.played_cards, self.game_logic.mode):
                return False
        else:
            self.trick_suit = card.suit
            
        self.played_cards[self.active_player.number] = card
        self.active_player.hand.remove_card(card)
        
        if all(c is not None for c in self.played_cards):
            winner_index, c = self.game_logic.trick_decision(self.played_cards, self.starting_player_index)
            print(f"Card won {c}")
            for won_card in self.played_cards:
                self.players[winner_index].add_won_card(won_card)
                
            self.played_cards = self.num_players * [None]
            self.trick_suit = None
            self.starting_player_index = winner_index
                
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
            winner_index, c = self.game_logic.trick_decision(self.played_cards, self.starting_player_index)
            print(f"Card won {c}")
            self.played_cards = self.num_players * [None]
            self.trick_suit = None
            self.starting_player_index = winner_index
            
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
                return "Licitátor vyhrál!"
            else:
                return "Licitátor prohrál!"
        
        players_points = 0
        licitator_points = self.licitator.calculate_hand(self.game_logic.mode)
        for player in self.players:
            if player != self.licitator:
                players_points += player.calculate_hand(self.game_logic.mode)
                
        if licitator_points > players_points:
            return f"Vyhrál Licitátor v poměru {licitator_points} : {players_points}"
            
        return f"Vyhrál/i hráč/i v poměru {players_points} : {licitator_points}"
    