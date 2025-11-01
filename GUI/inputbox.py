import pygame

WHITE = (255, 255, 255)
BLACK = (0, 0, 0)
GRAY = (200, 200, 200)
DARK_GRAY = (100, 100, 100)
LIGHT_GRAY = (240, 240, 240)
GREEN = (34, 139, 34)
YELLOW = (255, 255, 0)
DARK_GREEN = (0, 100, 0)
RED = (220, 20, 60)
BLUE = (70, 130, 180)
GOLD = (255, 215, 0)

class InputBox:
    """Třída pro vstupní pole v lobby."""
    def __init__(self, x, y, width, height, label, default_text=""):
        self.rect = pygame.Rect(x, y, width, height)
        self.color_inactive = GRAY
        self.color_active = BLACK
        self.color = self.color_inactive
        self.text = default_text
        self.label = label
        self.active = False
        self.font = pygame.font.Font(None, 32)
        
    def handle_event(self, event):
        if event.type == pygame.MOUSEBUTTONDOWN:
            self.active = self.rect.collidepoint(event.pos)
            self.color = self.color_active if self.active else self.color_inactive
        
        if event.type == pygame.KEYDOWN and self.active:
            if event.key == pygame.K_BACKSPACE:
                self.text = self.text[:-1]
            elif event.key == pygame.K_RETURN:
                return True
            else:
                if len(self.text) < 30:
                    self.text += event.unicode
        return False
    
    def draw(self, screen):
        # Vykresli label
        label_surf = self.font.render(self.label, True, YELLOW)
        screen.blit(label_surf, (self.rect.x, self.rect.y - 35))
        
        # Vykresli input box
        pygame.draw.rect(screen, WHITE, self.rect)
        pygame.draw.rect(screen, self.color, self.rect, 2)
        
        # Vykresli text
        txt_surface = self.font.render(self.text, True, BLACK)
        screen.blit(txt_surface, (self.rect.x + 10, self.rect.y + 10))