from cards import Card, card_mappping, Mode, CardRanks, CardSuits

TRUMPH_CARDS = 7 
class Hand: 
    def __init__(self): 
        self.cards = [] 
        self.won_cards = [] 
        self.points = 0 
    
    def check_right_input(self, card_input: str): 
        try: 
            card_mappping(card_input) 
            return False 
        except ValueError: 
            return False
        
    def find_card_in_hand(self, card_input: Card) -> bool: 
        for card in self.cards: 
            if card_input.__eq__(card): 
                return True 
        return False 
        
    def find_card_by_rank(self, rank: CardRanks, mode: Mode) -> bool: 
        for card in self.cards: 
            if card.get_value(mode) == rank.value: 
                return True 
        return False 
        
    def find_card_by_suit(self, suit: CardSuits) -> bool: 
        for card in self.cards: 
            if card.suit == suit: 
             return True 
        return False 
    
    def remove_card(self, card_to_remove: Card) -> None: 
        self.cards.remove(card_to_remove) 
        
    def add_card(self, card_to_add: Card) -> None: 
        self.cards.append(card_to_add) 
    
    def add_won_card(self, card: Card) -> None: 
        self.won_cards.append(card) 
        
    def calculate_hand(self): 
        for card in self.won_cards: 
            if card.rank == 7 or card.rank == 8: 
                self.points += 10
             
class Player: 
    def __init__(self, number: int): 
        self.number = number 
        self.hand = Hand() 
    
    def add_card(self, card: Card): 
        """Adds card to player from deck.""" 
        self.hand.add_card(card) 
        
    def add_won_card(self, card: Card): 
        """Moves won card into player's winning cards pack.""" 
        self.hand.add_won_card(card) 
        
    def has_card_in_hand(self) -> bool: 
        """Zkontroluje jestli má hráč ještě čím hrát.""" 
        return True if self.hand.cards else False 
    
    def has_won(self): 
        """Vyhrál hráč štych?""" 
        return True if self.hand.won_cards else False 
    
    def choose_card(self) -> Card: 
        """Allows the player to select and play a card with full validation.""" 
        # Inicializace proměnné played_card = None # Opakování smyčky, dokud není platná karta v ruce 
        while True: 
            played_card_input = input("Kterou kartu chcete zahrát?\n> ") 
            try: # Pokusí se namapovat vstup na objekt karty 
                played_card = card_mappping(played_card_input) 
                # Zkontroluje, zda je karta v ruce 
                if self.hand.find_card_in_hand(played_card): 
                    print(f"Zahrál si {played_card}") 
                    break 
                # Pokud je vše v pořádku, ukončí se smyčka 
                else: print("Karta není ve tvém balíčku karet! Zkus to znovu.") 
            except ValueError: # Pokud dojde k chybě při mapování (neplatný formát), 
                # vytiskne se chybová zpráva 
                print("Zadal jsi špatný kód karty! Zkus to znovu.") 
        return played_card 
    
    def play(self, 
             trick_suit: CardSuits|None, 
             trumph: CardSuits,
             played_cards: list[Card], 
             mode: Mode) -> Card:
        """Ask player for a card then remove from the hand and returns it"""
        played_card = self.choose_card()
        self.hand.remove_card(played_card)
        if trick_suit:
            while not self.check_played_card(trick_suit, trumph, played_card, played_cards, mode):
                self.hand.add_card(played_card)
                played_card = self.choose_card() 
                self.hand.remove_card(played_card)
        return played_card 
    
    def show_first_seven(self): 
        trumph_cards = [] 
        for i in range(TRUMPH_CARDS): 
            trumph_cards.append(self.hand.cards[i])
        return f"Player #{self.number}: {', '.join(str(card) for card in trumph_cards)}" 
    
    def calculate_hand(self) -> int: 
        return self.hand.points
    
    def check_played_card(self, trick_suit: CardSuits, trumph: CardSuits,
                        played_card: Card, played_cards: list[Card], mode: Mode) -> bool:
        """
        Ověří, zda hráč zahrál kartu podle pravidel mariáše.
        
        Args:
            trick_suit (CardSuits): barva první zahrané karty ve štychu
            trumph (CardSuits): trumfová barva
            played_card (Card): karta, kterou hráč chce zahrát
            played_cards (list[Card]): karty, které už byly zahrány ve štychu
            mode (Mode): aktuální mód hry

        Returns:
            bool: True = správně zahraná karta, False = porušení pravidel
        """

        # --- 1. má hráč stejnou barvu jako je povinná? ---
        if self.hand.find_card_by_suit(trick_suit):
            if played_card.suit != trick_suit:
                print("Musíš zahrát stejnou barvu!")
                return False  # musí dodržet barvu
            # --- 2. pokud už padla vyšší karta v barvě, musí ji přebít, pokud může ---
            highest_in_suit = max(
                [c for c in played_cards if c.suit == trick_suit],
                key=lambda c: c.get_value(mode),
                default=None
            )
            if highest_in_suit and played_card.get_value(mode) < highest_in_suit.get_value(mode):
                # hráč má vyšší kartu v barvě?
                if any(c.suit == trick_suit and c.get_value(mode) > highest_in_suit.get_value(mode)
                    for c in self.hand.cards):
                    print("Musíš zahrát vyšší kartu!")
                    return False
            return True

        # --- 3. hráč nemá barvu, ale má trumf ---
        if self.hand.find_card_by_suit(trumph):
            if played_card.suit != trumph:
                print("Musíš zahrát trumf!")
                return False
            # musí přebít trumf, pokud je už ve štychu
            highest_trump = max(
                [c for c in played_cards if c.suit == trumph],
                key=lambda c: c.get_value(mode),
                default=None
            )
            if highest_trump and played_card.get_value(mode) < highest_trump.get_value(mode):
                if any(c.suit == trumph and c.get_value(mode) > highest_trump.get_value(mode)
                    for c in self.hand.cards):
                    print("Musíš zahrát vyšší kartu!")
                    return False
            return True

        # --- 4. nemá ani barvu, ani trumf → může zahrát cokoli ---
        return True
            
    def __str__(self):
        return f"Player #{self.number}: {', '.join(str(card) for card in self.hand.cards)}"