# marias_gui.py
"""
Jednoduchá GUI kostra pro hru (převod z konzole do PyQt6).
Tento skript poskytuje základní okno s lobbym, zobrazením hráčů a jejich rukou,
možností zvolit kartu a zahrát štych lokálně (bez serveru). Slouží jako proof-of-concept
pro přepnutí hry z konzole do grafiky; herní logika je zjednodušena.

Závislosti:
    pip install PyQt6

Spuštění:
    python marias_gui.py

Poznámky:
 - Tento soubor obsahuje zjednodušenou verzi tříd Card/Deck/Player/Game ze zadaní.
 - Neimplementuje všechny herní módy plně (betl/durch/sedma jsou jen kostra).
 - Cílem: ukázat, jak GUI komunikovat s herní logikou, a kde připojit síťovou vrstvu.
"""
from __future__ import annotations
import sys
import random
from enum import Enum
from typing import List, Optional

from PyQt6.QtWidgets import (
    QApplication, QWidget, QMainWindow, QLabel, QPushButton, QListWidget,
    QVBoxLayout, QHBoxLayout, QMessageBox, QListWidgetItem, QComboBox
)
from PyQt6.QtCore import Qt

# ---------------------- Herní model (zjednodušeně) ----------------------
class Mode(Enum):
    HRA = 1
    SEDMA = 2
    BETL = 4
    DURCH = 5

class Card:
    def __init__(self, rank: str, suit: str):
        self.rank = rank
        self.suit = suit

    def __str__(self):
        return f"{self.rank}{self.suit}"

    def __repr__(self):
        return str(self)

    def get_value(self, mode: Mode) -> int:
        # velmi zjednodušené pořadí
        order = {'a':8,'k':7,'q':6,'j':5,'10':4,'9':3,'8':2,'7':1}
        val = order.get(self.rank, 0)
        # při betl/durch by mohl být 10 pod 9, ale pro demo to necháme
        return val

SUITS = ['♥','♦','♣','♠']
RANKS = ['a','k','q','j','10','9','8','7']

class Deck:
    def __init__(self):
        self.cards: List[Card] = []
        self._init()

    def _init(self):
        self.cards = [Card(r,s) for s in SUITS for r in RANKS]

    def shuffle(self):
        random.shuffle(self.cards)

    def deal(self) -> Optional[Card]:
        return self.cards.pop() if self.cards else None

class Player:
    def __init__(self, number:int):
        self.number = number
        self.cards: List[Card] = []
        self.won_cards: List[Card] = []

    def __str__(self):
        return f"Player #{self.number}"

    def add_card(self, c:Card):
        self.cards.append(c)

    def play_card(self, index:int) -> Card:
        return self.cards.pop(index)

# Jednoduchá lokální hra pro demo
class Game:
    def __init__(self, num_players:int=3):
        self.num_players = num_players
        self.players = [Player(i) for i in range(num_players)]
        self.deck = Deck()
        self.deck.shuffle()
        self.licitator = self.players[0]
        self.mode = Mode.HRA
        self.starting_player_index = 0

    def deal(self):
        # velmi jednoduché rozdání: rovnoměrně odkudkoli
        while self.deck.cards:
            for p in self.players:
                c = self.deck.deal()
                if c:
                    p.add_card(c)

    def get_current_player(self) -> Player:
        return self.players[self.starting_player_index % self.num_players]

    def advance_player(self):
        self.starting_player_index = (self.starting_player_index + 1) % self.num_players

# ---------------------- GUI ----------------------
class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Marias - GUI kostra")
        self.resize(800, 600)

        # Herní instance
        self.game = Game(num_players=3)

        # Layouty
        central = QWidget()
        self.setCentralWidget(central)
        main_layout = QVBoxLayout()
        central.setLayout(main_layout)

        # Horní panel: tlačítka pro start/hra
        top_bar = QHBoxLayout()
        main_layout.addLayout(top_bar)

        self.btn_new = QPushButton("Nová hra")
        self.btn_new.clicked.connect(self.on_new_game)
        top_bar.addWidget(self.btn_new)

        top_bar.addStretch()

        self.mode_combo = QComboBox()
        self.mode_combo.addItems([m.name for m in Mode])
        self.mode_combo.setCurrentText(Mode.HRA.name)
        self.mode_combo.currentTextChanged.connect(self.on_mode_changed)
        top_bar.addWidget(QLabel("Mód:"))
        top_bar.addWidget(self.mode_combo)

        # Střední část: seznam hráčů + detail ruky
        mid = QHBoxLayout()
        main_layout.addLayout(mid)

        # Seznam hráčů
        left = QVBoxLayout()
        mid.addLayout(left, 1)
        left.addWidget(QLabel("Hráči:"))
        self.player_list = QListWidget()
        self.player_list.currentRowChanged.connect(self.on_player_selected)
        left.addWidget(self.player_list)

        # Detail vybraného hráče
        right = QVBoxLayout()
        mid.addLayout(right, 2)
        right.addWidget(QLabel("Ruka hráče:"))
        self.hand_list = QListWidget()
        right.addWidget(self.hand_list)

        # Panel pro akce hráče
        action_bar = QHBoxLayout()
        right.addLayout(action_bar)

        self.btn_play = QPushButton("Zahrát kartu")
        self.btn_play.clicked.connect(self.on_play_card)
        action_bar.addWidget(self.btn_play)

        self.btn_show_hand = QPushButton("Ukázat prvních 7")
        self.btn_show_hand.clicked.connect(self.on_show_first_seven)
        action_bar.addWidget(self.btn_show_hand)

        action_bar.addStretch()

        # Zobrazení posledního štychu
        main_layout.addWidget(QLabel("Poslední zahrané karty:"))
        self.last_trick = QListWidget()
        main_layout.addWidget(self.last_trick)

        # Stavová lišta
        self.status = QLabel("")
        main_layout.addWidget(self.status)

        # Inicializace GUI stavu
        self.on_new_game()

    # ---------- Handlery tlačítek ----------
    def on_new_game(self):
        self.game = Game(num_players=3)
        self.game.deal()
        self.refresh_player_list()
        self.status.setText("Nová hra rozdána.")
        self.last_trick.clear()
        self.player_list.setCurrentRow(0)

    def on_mode_changed(self, text:str):
        self.game.mode = Mode[text]
        self.status.setText(f"Změněn mód na: {text}")

    def refresh_player_list(self):
        self.player_list.clear()
        for p in self.game.players:
            item = QListWidgetItem(f"Player #{p.number} ({len(p.cards)} karet)")
            self.player_list.addItem(item)

    def on_player_selected(self, row:int):
        if row < 0:
            return
        player = self.game.players[row]
        self.refresh_hand(player)

    def refresh_hand(self, player:Player):
        self.hand_list.clear()
        for c in player.cards:
            self.hand_list.addItem(str(c))

    def on_play_card(self):
        row = self.player_list.currentRow()
        if row < 0:
            QMessageBox.warning(self, "Chyba", "Vyber hráče, který má zahrát")
            return
        player = self.game.players[row]
        card_row = self.hand_list.currentRow()
        if card_row < 0:
            QMessageBox.warning(self, "Chyba", "Vyber kartu z ruky")
            return
        card = player.play_card(card_row)
        # Přidáme kartu do posledního štychu (pro demo ukážeme jako jednotlivé karty)
        self.last_trick.addItem(f"Player #{player.number}: {card}")
        self.status.setText(f"Player #{player.number} zahrál {card}")
        self.refresh_hand(player)
        self.refresh_player_list()

    def on_show_first_seven(self):
        row = self.player_list.currentRow()
        if row < 0:
            return
        player = self.game.players[row]
        first7 = player.cards[:7]
        QMessageBox.information(self, "Prvních 7 karet", ", ".join(str(c) for c in first7) if first7 else "Žádné karty")

# ---------------------- Spuštění aplikace ----------------------
if __name__ == '__main__':
    app = QApplication(sys.argv)
    w = MainWindow()
    w.show()
    sys.exit(app.exec())
