from src.gui import GUI
import pygame
import sys

if __name__ == "__main__":
    gui = GUI()

    while True:
        gui.draw()
        
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                pygame.quit()
                sys.exit()
            gui.handle_event(event)
            
        gui.clock.tick(30)