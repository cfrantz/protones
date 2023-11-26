######################################################################
# Simple cheats script for Zelda 2
#
# This script serves as an example of how to hook into ProtoNES
# with python.
######################################################################
from application import gui
from .. import plugin
from . import cheats
from . import execution
from . import hitbox
from . import random

# Subclass the EmulatorHooks class
class Zelda2(plugin.Plugin):

    def __init__(self, app):
        # Call the superclass' init first.
        super().__init__(app)
        self.rng = random.RNG(app)
        self.cheats = cheats.Cheats(app)
        self.execution = execution.Execution(app)
        self.hitbox_visible = False
        self.hitbox = [hitbox.LinkHitbox(app)]
        self.hitbox.extend(hitbox.EnemyHitbox(app, i) for i in range(13))

    def z2goto(self, a, b, c, d, e, f, g):
        """Go to a sideview area in Zelda 2"""
        mem = self.app.nes.mem
        mem[0x769] = a
        mem[0x706] = b
        mem[0x707] = c
        mem[0x56b] = d
        mem[0x56c] = e
        mem[0x748] = f
        mem[0x561] = g
        mem[0x736] = 0

    def DisableEncounters(self, dis):
        if dis:
            # nop, nop, sec.  carry flag causes jump away from enc routine
            debug.WritePrgBank(0, 0x83c4, [0xea, 0xea, 0x38])
        else:
            # jsr blocked_by_tile_routine - checks what tile we'll take the
            # encounter for.
            debug.WritePrgBank(0, 0x83c4, [0x20, 0x0f, 0x87])

    def menu_bar(self):
        """Hook into the menubar and some menus."""
        if gui.begin_menu("Goto"):
            if gui.menu_item("Palace 1", ""):
                self.z2goto(4, 0, 3, 4, 0, 52, 0)
            elif gui.menu_item("Palace 2", ""):
                self.z2goto(4, 0, 3, 4, 1, 53, 0xe)
            elif gui.menu_item("Palace 3", ""):
                self.z2goto(4, 0, 4, 5, 2, 54, 0)
            elif gui.menu_item("Palace 4", ""):
                self.z2goto(4, 1, 4, 8, 0, 52, 0xf)
            elif gui.menu_item("Palace 5", ""):
                self.z2goto(4, 2, 3, 8, 0, 52, 0x23)
            elif gui.menu_item("Palace 6", ""):
                self.z2goto(4, 2, 4, 8, 1, 53, 0x24)
            elif gui.menu_item("Palace 7", ""):
                self.z2goto(5, 2, 5, 9, 2, 54, 0)
            elif gui.menu_item("Rauru", ""):
                self.z2goto(3, 0, 1, 0, 0xf8, 45, 2)
            elif gui.menu_item("Ruto", ""):
                self.z2goto(3, 0, 1, 1, 0xf8, 47, 5)
            elif gui.menu_item("Saria", ""):
                self.z2goto(3, 0, 1, 2, 0xf8, 49, 8)
            elif gui.menu_item("Mido", ""):
                self.z2goto(3, 0, 1, 3, 0xf8, 51, 11)
            elif gui.menu_item("Nabooru", ""):
                self.z2goto(3, 2, 2, 4, 0xf8, 45, 14)
            elif gui.menu_item("Darunia", ""):
                self.z2goto(3, 2, 2, 5, 0xf8, 47, 17)
            elif gui.menu_item("New Kasuto", ""):
                self.z2goto(3, 2, 2, 6, 0xf8, 49, 20)
            elif gui.menu_item("Old Kasuto", ""):
                self.z2goto(3, 2, 2, 7, 0xf8, 51, 23)
            elif gui.menu_item("T-Bird Fight", ""):
                self.z2goto(5, 2, 5, 9, 2, 54, 53)
            elif gui.menu_item("Dark Link Fight", ""):
                self.z2goto(5, 2, 5, 9, 2, 54, 54)
            gui.end_menu()
        if gui.begin_menu("Cheats"):
            if gui.menu_item("Cheats", "", selected=self.cheats.visible):
                self.cheats.visible = not self.cheats.visible
            #if gui.menu_item("Disable Encounters", "", selected=self.disable_encounters):
            #    self.DisableEncounters(self.disable_encounters.value)
            if gui.menu_item("Show Hitboxes", "", selected=self.hitbox_visible):
                self.hitbox_visible = not self.hitbox_visible
            #if gui.menu_item("Memory Values", "", selected=self.memory.visible):
            #    pass
            #if gui.menu_item("CFplayer Values", "", selected=self.cfplayer.visible):
            #    pass
            if gui.menu_item("RNG", "", selected=self.rng.visible):
                self.rng.visible = not self.rng.visible
            if gui.menu_item("Exec Profile", "", selected=self.execution.visible):
                self.execution.enable(not self.execution.visible)
            gui.end_menu()

    def run_per_frame(self):
        """Called for every frame."""
        for h in self.hitbox:
            h.update()

    def draw(self):
        """Called on each frame to draw your own GUIs."""
        self.cheats.draw()
        self.execution.draw()
        #self.memory.Draw()
        #self.cfplayer.Draw()
        self.rng.draw()

    def draw_image(self):
        if self.hitbox_visible:
            for h in self.hitbox:
                h.draw_image()

def create(app):
    return Zelda2(app)
