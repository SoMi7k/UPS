import pygame
from enum import Enum
from typing import Tuple
from .Validator import InputValidator

class Color(Tuple, Enum):
    WHITE = (255, 255, 255)
    BLACK = (0, 0, 0)
    GRAY = (200, 200, 200)
    DARK_GRAY = (100, 100, 100)
    LIGHT_GRAY = (240, 240, 240)
    DARK_YELLOW = (175, 175, 20)
    DARK_GREEN = (0, 100, 0)
    #RED = (220, 20, 60)
    BLUE = (70, 130, 180)
    GOLD = (255, 215, 0)
    GREEN = (46, 204, 113)
    YELLOW = (241, 196, 15)
    RED = (231, 76, 60)
    DARK_RED = (192, 57, 43)

class Button:
    """Třída pro tlačítko."""
    def __init__(self, x, y, width, height, text, color=Color.GREEN, hover_color=Color.DARK_GREEN):
        self.rect = pygame.Rect(x, y, width, height)
        self.text = text
        self.color = color
        self.hover_color = hover_color
        self.current_color = color
        self.font = pygame.font.Font(None, 36)
        self.border_radius = 10
        
    def draw(self, screen):
        mouse_pos = pygame.mouse.get_pos()
        if self.rect.collidepoint(mouse_pos):
            self.current_color = self.hover_color
        else:
            self.current_color = self.color
            
        pygame.draw.rect(screen, self.current_color, self.rect, border_radius=self.border_radius)
        pygame.draw.rect(screen, Color.BLACK, self.rect, 2, border_radius=self.border_radius)
        
        text_surf = self.font.render(self.text, True, Color.WHITE)
        text_rect = text_surf.get_rect(center=self.rect.center)
        screen.blit(text_surf, text_rect)
    
    def is_clicked(self, event):
        if event.type == pygame.MOUSEBUTTONDOWN:
            return self.rect.collidepoint(event.pos)
        return False
    
class InputBox:
    """Třída pro vstupní pole v lobby s real-time validací."""
    def __init__(self, x, y, width, height, label, default_text="", input_type="text"):
        self.rect = pygame.Rect(x, y, width, height)
        self.color_inactive = Color.GRAY
        self.color_active = Color.BLACK
        self.color_error = Color.RED
        self.color = self.color_inactive
        self.text = default_text
        self.label = label
        self.active = False
        self.font = pygame.font.Font(None, 32)
        self.font_small = pygame.font.Font(None, 20)
        
        # 🆕 Typ vstupu a validace
        self.input_type = input_type  # "ip", "port", "nickname", "text"
        self.error_message = ""
        
    def handle_event(self, event):
        if event.type == pygame.MOUSEBUTTONDOWN:
            self.active = self.rect.collidepoint(event.pos)
            self.color = self.color_active if self.active else self.color_inactive
        
        if event.type == pygame.KEYDOWN and self.active:
            if event.key == pygame.K_BACKSPACE:
                self.text = self.text[:-1]
                self.validate()  # 🆕 Validace po každé změně
            elif event.key == pygame.K_RETURN:
                return True
            else:
                # Omez délku podle typu
                max_len = 30
                if self.input_type == "nickname":
                    max_len = 12
                elif self.input_type == "ip":
                    max_len = 15
                elif self.input_type == "port":
                    max_len = 5
                
                if len(self.text) < max_len:
                    self.text += event.unicode
                    self.validate()  # 🆕 Validace po každé změně
        return False
    
    def validate(self):
        """Validuje obsah podle typu inputu."""
        if self.input_type == "nickname":
            valid, error = InputValidator.validate_nickname(self.text)
            self.error_message = error if not valid else ""
            self.color = self.color_error if not valid else (self.color_active if self.active else self.color_inactive)
        
        elif self.input_type == "port":
            valid, error, _ = InputValidator.validate_port(self.text)
            self.error_message = error if not valid else ""
            self.color = self.color_error if not valid else (self.color_active if self.active else self.color_inactive)
        
        elif self.input_type == "ip":
            valid, error = InputValidator.validate_ip(self.text)
            self.error_message = error if not valid else ""
            self.color = self.color_error if not valid else (self.color_active if self.active else self.color_inactive)
    
    def draw(self, screen):
        # Vykresli label
        label_surf = self.font.render(self.label, True, Color.YELLOW)
        screen.blit(label_surf, (self.rect.x, self.rect.y - 35))
        
        # Vykresli input box
        pygame.draw.rect(screen, Color.WHITE, self.rect)
        pygame.draw.rect(screen, self.color, self.rect, 2)
        
        # Vykresli text
        txt_surface = self.font.render(self.text, True, Color.BLACK)
        screen.blit(txt_surface, (self.rect.x + 10, self.rect.y + 10))
        
        # 🆕 Vykresli chybovou zprávu pod inputem
        if self.error_message:
            error_surf = self.font_small.render(self.error_message, True, Color.RED)
            screen.blit(error_surf, (self.rect.x, self.rect.y + self.rect.height + 5))

class HelpButton:
    """Ikona otazníku v levém horním rohu."""
    def __init__(self, x=20, y=20, size=50):
        self.rect = pygame.Rect(x, y, size, size)
        self.size = size
        self.color = Color.BLUE
        self.hover_color = Color.YELLOW
        self.current_color = self.color
        self.font = pygame.font.Font(None, 40)
        
    def draw(self, screen):
        """Vykreslí kruhovou ikonu s otazníkem."""
        mouse_pos = pygame.mouse.get_pos()
        if self.rect.collidepoint(mouse_pos):
            self.current_color = self.hover_color
        else:
            self.current_color = self.color
        
        # Kruh s otazníkem
        center = self.rect.center
        pygame.draw.circle(screen, self.current_color, center, self.size // 2)
        pygame.draw.circle(screen, Color.WHITE, center, self.size // 2, 3)
        
        # Otazník uprostřed
        question_mark = self.font.render("?", True, Color.WHITE)
        question_rect = question_mark.get_rect(center=center)
        screen.blit(question_mark, question_rect)
    
    def is_clicked(self, event):
        """Zkontroluje, zda bylo na tlačítko kliknuto."""
        if event.type == pygame.MOUSEBUTTONDOWN:
            return self.rect.collidepoint(event.pos)
        return False