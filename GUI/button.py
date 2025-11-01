import pygame

WHITE = (255, 255, 255)
BLACK = (0, 0, 0)
GRAY = (200, 200, 200)
DARK_GRAY = (100, 100, 100)
LIGHT_GRAY = (240, 240, 240)
GREEN = (34, 139, 34)
DARK_GREEN = (0, 100, 0)
RED = (220, 20, 60)
BLUE = (70, 130, 180)
GOLD = (255, 215, 0)

class Button:
    """Třída pro tlačítko."""
    def __init__(self, x, y, width, height, text, color=GREEN, hover_color=DARK_GREEN):
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
        pygame.draw.rect(screen, BLACK, self.rect, 2, border_radius=self.border_radius)
        
        text_surf = self.font.render(self.text, True, WHITE)
        text_rect = text_surf.get_rect(center=self.rect.center)
        screen.blit(text_surf, text_rect)
    
    def is_clicked(self, event):
        if event.type == pygame.MOUSEBUTTONDOWN:
            return self.rect.collidepoint(event.pos)
        return False
