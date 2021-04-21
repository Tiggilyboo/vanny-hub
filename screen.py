
class Screen:
    def draw(self, display):
        display.clear()
        display.text(self.title, int(display.width / 2) - len(self.title) * 4, 0)

        if self.screen_left != None:
            display.text('<', 0, 0)
        if self.screen_right != None:
            display.text('>', display.width - 8, 0)
            
        display.show()
        
    def set_navigation(self, left=None, right=None):
        self.screen_left = left
        self.screen_right = right
        
    def __init__(self, title):
        self.title = title
        self.screen_left = None
        self.screen_right = None
