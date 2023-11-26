import application

class Plugin(object):
    def __init__(self, app):
        self.app = app

    def run_per_frame(self):
        pass

    def draw(self):
        pass

    def menu_bar(self):
        pass

    def menu(self, name):
        pass
