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
        self.result = None
        self.state = State.ROZDÁNÍ_KARET
        self.talon = 0
        self.higher_game = False
        self.higher_player = None
        self.trick_suit = None
        self.starting_player_index = self.licitator
        
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
        

    def accept_mode(self, response: str, player: Player) -> bool:
        # Ptá se hráče na hru -> dobrá/špatná
        if response == "Dobrý":
            print(f"Player #{player.number}: Dobrý")
            return True
        else:
            if self.game_logic.mode.value == 1:
                # TODO INPUT
                mode = input("Hra: [b|d]\n> ")
                if mode == "Špatný":
                    print(f"Player #{player.number}: Špatný - Betl")
                    self.mode = Mode.BETL
                else:
                    print(f"Player #{player.number}: Špatný - Durch")
                    self.mode = Mode.DURCH
            else:
                print(f"Player #{player.number}: Špatný - Durch")
                self.mode = Mode.DURCH
            
            return False
            
    def licitating(self):
        # TODO na serveru bude potřeba přidat komu se bude co posílat
        print("Licitátor vybírá trumf.")
        self.gui.draw_cards(self.licitator.pick_cards(12))
        self.gui.draw_selection_buttons(self.licitator.pick_cards(ROZDAVANI_KARET[0]))
        self.game_logic.choose_trumph(self.licitator)
        print("Licitátor odhazuje karty do talonu.")
        print(self.licitator)
        self.remove_cards(self.licitator)
        print("Licitátor vybírá hru.")
        self.game_logic.choose_mode()
        print(f"{self.game_logic.mode.name} {self.game_logic.trumph.name}")
        
        higher_game = False
        for player in self.players:
            if player != self.licitator:
                print(player)
                if self.game_logic.mode != Mode.DURCH:
                    if not self.game_logic.accept_mode(player):
                        self.licitator = player
                        higher_game = True
                else:
                    print(f"Player #{player.number}: Dobrý")
        
        if higher_game:
            self.game_logic.higher_game_call(self.licitator)
            
        print(self.game_logic.mode.name + " " + self.game_logic.trumph.name)

        
    def show_played_cards(self, cards: list[Card]):
        for i, card in enumerate(cards):
            print(f"Player #{self.players[i].number} hrál {card}")

    def play_stych(self, starting_player_index: int):
        played_cards: list[Card] = []
        for i in range(self.num_players):
            player = self.players[(starting_player_index + i) % self.num_players]
            print(f"Player #{player.number} hraje:")
            print(player)
            if i == 0:
                self.played_cards[player.number] = player.play(None, self.game_logic.trumph, played_cards, self.game_logic.mode)
                trick_suit = self.played_cards[player.number].suit
                played_cards.append(self.played_cards[player.number])
            else:
                self.played_cards[player.number] = player.play(trick_suit, self.game_logic.trumph, played_cards, self.game_logic.mode)
                played_cards.append(self.played_cards[player.number])
            print("")
    
        winner_index, winner_card = self.game_logic.trick_decision(self.played_cards, starting_player_index)
        
        # Výsledek štychu
        self.show_played_cards(self.played_cards)
        self.played_cards = self.num_players * [None]
        print(f"Štych vyhrál Player #{self.players[winner_index].number} s kartou {winner_card}\n")
        
        # Přesun karet do "vítězného balíčku"
        for card in self.played_cards:
            self.players[winner_index].add_won_card(card)
            
        return winner_index
    
    def hra_sedma(self):
        starting_player_index = self.licitator.number
        while self.licitator.has_card_in_hand():
            winner_index, _ = self.play_stych(starting_player_index)
            starting_player_index = winner_index
        
        self.game_result()
        
    def betl_durch(self):
        starting_player_index = self.licitator.number
        while self.licitator.has_card_in_hand():
            winner_index, _ = self.play_stych(starting_player_index)
            starting_player_index = winner_index
            
            if self.game_logic.mode == Mode.BETL:
                if winner_index == self.licitator.number:
                    print("Licitátor prohrál!")
                    self.result = False
                    return
            else:          
                if winner_index != self.licitator.number:
                    print("Licitátor prohrál!")
                    self.result = False
                    return
            
        print("Licitátor vyhrál!")
        self.result = True    

    def licitator_won(self) -> bool:
        """Pokud licitátor vyhrál True, jinak False."""
        return self.result
            
    def marias(self):
        self.deal_cards()
        #self.licitating()
        #self.play()
        
if __name__ == "__main__":
    game = Game()
    game.marias()