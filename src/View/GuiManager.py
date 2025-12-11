import pygame
import math
from .Obsticles import Button, InputBox, Color

IMG_DIR = "images/wooden_table.jpg"

class GuiManager:
    def __init__(self):
        # Konstanty
        self.width = 1200
        self.height = 700
        self.card_width = 70
        self.card_height = 100
        
        # Inicializace
        pygame.init()
        self.screen = pygame.display.set_mode((self.width, self.height))
        pygame.display.set_caption("Mariáš - Multiplayer")
        
        # Fonty
        self.title_font = pygame.font.Font(None, 72)
        self.font_large = pygame.font.Font(None, 48)
        self.font_medium = pygame.font.Font(None, 36)
        self.font_small = pygame.font.Font(None, 28)
        self.font = pygame.font.SysFont("Arial", 28, bold=True)
        
        # Lobby komponenty
        self.setup_lobby()  
        
        # Pozadí
        self.background = self.create_background()
        
    def create_background(self):
        """Vytvoří gradient pozadí nebo načte obrázek."""
        try:
            # Zkusíme načíst obrázek
            image = pygame.image.load(IMG_DIR).convert_alpha()
            return pygame.transform.scale(image, (self.width, self.height))
        except Exception as _:
            # Fallback na gradient
            print("⚠ Nepodařilo se načíst pozadí, použiji gradient")
            background = pygame.Surface((self.width, self.height))
            for y in range(self.height):
                color_value = int(20 + (y / self.height) * 40)
                pygame.draw.line(background, (color_value, color_value + 20, color_value + 10), 
                                 (0, y), (self.width, y))
            return background
        
    # ============================================================
    # LOBBY - Vykreslení úvodního lobby
    # ============================================================
    
    def setup_lobby(self):
        """Nastaví komponenty lobby."""
        center_x = self.width // 2
        
        # Input boxy
        self.ip_input = InputBox(center_x - 150, 250, 300, 50, "Server IP:", "127.0.0.1")
        self.port_input = InputBox(center_x - 150, 350, 300, 50, "Port:", "10000")
        self.nickname_input = InputBox(center_x - 150, 450, 300, 50, "Nickname:", "Player")
        
        # Tlačítka
        # Původní: self.connect_button = Button(center_x - 100, 550, 200, 60, "Připojit", YELLOW, DARK_YELLOW)
        self.connect_button = Button(center_x - 100, 550, 200, 60, "Připojit", Color.YELLOW, Color.DARK_YELLOW)
        # Původní: self.quit_button = Button(center_x - 100, 620, 200, 60, "Ukončit", RED, (180, 0, 0))
        self.quit_button = Button(center_x - 100, 620, 200, 60, "Ukončit", Color.RED, (180, 0, 0))
        self.back_button = Button(center_x - 100, 620, 200, 60, "Opustit hru", Color.BLUE, (0, 0, 255))
        
    # ============================================================
    # VYKRESLOVÁNÍ - Draw funkce pro různé stavy
    # ============================================================
    
    def draw_text(self, text, font, color, x=None, y=None, center=False):
        """Vykreslí text s anti-aliasingem."""
        text_surface = font.render(text, True, color)
        text_rect = text_surface.get_rect()
        
        if center:
            text_rect.center = (self.width // 2, y if y else self.height // 2)
        else:
            text_rect.topleft = (x if x else 0, y if y else 0)
        
        self.screen.blit(text_surface, text_rect)
        return text_rect
    
    
    def draw_lobby(self):
        """Vykreslí lobby obrazovku."""
        self.screen.blit(self.background, (0, 0))
        
        # Nadpis s stínem
        self.draw_text("MARIÁŠ", self.title_font, Color.DARK_GRAY, y=83, center=True)
        self.draw_text("MARIÁŠ", self.title_font, Color.GOLD, y=80, center=True)
        self.draw_text("Online Multiplayer", self.font_medium, Color.WHITE, y=150, center=True)
        
        # Input boxy a tlačítka
        self.ip_input.draw(self.screen)
        self.port_input.draw(self.screen)
        self.nickname_input.draw(self.screen)
        self.connect_button.draw(self.screen)
        self.quit_button.draw(self.screen)
    
    def draw_connecting(self):
        """Vykreslí obrazovku připojování."""
        self.screen.blit(self.background, (0, 0))
        
        dots = "." * ((pygame.time.get_ticks() // 500) % 4)
        self.draw_text(f"Připojuji{dots}", self.font_large, Color.WHITE, 
                             y=self.height // 2 - 50, center=True)
        
        self.draw_text("Probíhá příprava hry...", self.font_small, Color.GRAY,
                             y=self.height // 2 + 20, center=True)
    
    def draw_waiting(self, connected_players: int, required_players: int, player_number: int):
        """Vykreslí obrazovku čekání na ostatní hráče."""
        self.screen.blit(self.background, (0, 0))
        self.draw_text("Čekám na hráče", self.font_large, Color.WHITE,
                             y=self.height // 2 - 100, center=True)
        
        player_text = f"{connected_players} / {required_players}"
        self.draw_text(player_text, self.font_large, Color.GOLD,
                             y=self.height // 2 - 20, center=True)
        
        dots = "." * ((pygame.time.get_ticks() // 500) % 4)
        self.draw_text(dots, self.font_medium, Color.WHITE,
                             y=self.height // 2 + 100, center=True)
        
        if player_number is not None:
            info_text = f"Hráč #{player_number}"
            self.draw_text(info_text, self.font_small, Color.YELLOW,
                                 y=self.height - 50, center=True)
            
        self.back_button.draw(self.screen)
    
    def draw_reconnecting(self, attempt: int, max_attempts: int):
        """
        Vykreslí reconnecting obrazovku s animací a progress barem.
        
        Args:
            attempt: Aktuální pokus o reconnect
            max_attempts: Maximální počet pokusů
        """
        self.screen.fill(Color.DARK_GREEN)
        
        # === NADPIS ===
        title = self.title_font.render("Reconnecting...", True, Color.WHITE)
        title_rect = title.get_rect(center=(self.width // 2, 150))
        self.screen.blit(title, title_rect)
        
        # === ANIMACE TOČÍCÍHO SE KOLEČKA ===
        # Vytvoříme rotating loader
        center_x = self.width // 2
        center_y = 250
        radius = 40
        num_dots = 8
        
        # Animace pomocí time-based rotation
        time_ms = pygame.time.get_ticks()
        rotation = (time_ms / 10) % 360  # Rotace každých 10ms
        
        for i in range(num_dots):
            angle = math.radians(rotation + (i * 360 / num_dots))
            x = center_x + int(radius * math.cos(angle))
            y = center_y + int(radius * math.sin(angle))
            
            # Fade efekt - vzdálenější body jsou průhlednější
            alpha = int(255 * (1 - i / num_dots))
            dot_size = 8 - int(i / 2)  # Velikost se zmenšuje
            
            # Kreslíme kruh s alpha kanálem
            dot_surface = pygame.Surface((dot_size * 2, dot_size * 2), pygame.SRCALPHA)
            pygame.draw.circle(dot_surface, (Color.WHITE, alpha), (dot_size, dot_size), dot_size)
            self.screen.blit(dot_surface, (x - dot_size, y - dot_size))
        
        # === PROGRESS BAR ===
        bar_width = 400
        bar_height = 30
        bar_x = (self.width - bar_width) // 2
        bar_y = 350
        
        # Pozadí progress baru
        pygame.draw.rect(self.screen, Color.LIGHT_GRAY, 
                        (bar_x, bar_y, bar_width, bar_height), 
                        border_radius=15)
        
        # Vyplněná část
        if max_attempts > 0:
            progress = attempt / max_attempts
            fill_width = int(bar_width * progress)
            
            # Barva se mění s postupem času (zelená -> žlutá -> červená)
            if progress < 0.5:
                color = Color.GREEN
            elif progress < 0.8:
                color = Color.YELLOW
            else:
                color = Color.RED
            
            if fill_width > 0:
                pygame.draw.rect(self.screen, color,
                               (bar_x, bar_y, fill_width, bar_height),
                               border_radius=15)
        
        # Okraj progress baru
        pygame.draw.rect(self.screen, Color.WHITE,
                        (bar_x, bar_y, bar_width, bar_height),
                        width=2, border_radius=15)
        
        # === TEXT S POČTEM POKUSŮ ===
        attempt_text = self.font_medium.render(
            f"Attempt {attempt} / {max_attempts}",
            True, Color.WHITE
        )
        attempt_rect = attempt_text.get_rect(center=(self.width // 2, bar_y + bar_height + 40))
        self.screen.blit(attempt_text, attempt_rect)
        
        # === ČASOVÝ ODHAD ===
        remaining_seconds = max_attempts - attempt
        time_text = self.font_small.render(
            f"Time remaining: ~{remaining_seconds}s",
            True, Color.LIGHT_GRAY
        )
        time_rect = time_text.get_rect(center=(self.width // 2, bar_y + bar_height + 80))
        self.screen.blit(time_text, time_rect)
        
        # === INFO TEXT ===
        info_lines = [
            "Attempting to reconnect to the server...",
            "Please check your internet connection.",
        ]
        
        y_offset = 480
        for line in info_lines:
            info_text = self.font_small.render(line, True, Color.LIGHT_GRAY)
            info_rect = info_text.get_rect(center=(self.width // 2, y_offset))
            self.screen.blit(info_text, info_rect)
            y_offset += 30
        
        # === TLAČÍTKO CANCEL ===
        if hasattr(self, 'back_button'):
            self.back_button.draw(self.screen)
        else:
            # Vytvoříme cancel button pokud neexistuje
            cancel_button_rect = pygame.Rect(
                (self.width - 200) // 2,
                self.height - 100,
                200, 50
            )
            
            mouse_pos = pygame.mouse.get_pos()
            is_hover = cancel_button_rect.collidepoint(mouse_pos)
            
            button_color = Color.RED if is_hover else Color.DARK_RED
            pygame.draw.rect(self.screen, button_color, cancel_button_rect, border_radius=10)
            pygame.draw.rect(self.screen, Color.WHITE, cancel_button_rect, width=2, border_radius=10)
            
            cancel_text = self.font_medium.render("Cancel", True, Color.WHITE)
            cancel_rect = cancel_text.get_rect(center=cancel_button_rect.center)
            self.screen.blit(cancel_text, cancel_rect)