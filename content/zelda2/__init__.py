######################################################################
# Simple cheats script for Zelda 2
#
# This script serves as an example of how to hook into ProtoNES 
# with python.
######################################################################
import app
import bimpy
from . import cheats
from . import hitbox
from . import random
from . import memory
from .. import debug

# Subclass the EmulatorHooks class
class Zelda2(app.EmulatorHooks):

    def __init__(self, root=None):
        # Call the superclass' init first.
        super().__init__(root)
        # Then initialize our own local data
        self.cheats = cheats.Cheats(self.root)
        self.rng = random.RNG(self.root)
        self.memory = memory.Memory(self.root)
        self.hitbox_visible = bimpy.Bool()
        self.disable_encounters = bimpy.Bool()
        self.hitbox = [hitbox.LinkHitbox(self.root)]
        self.hitbox.extend(
                hitbox.EnemyHitbox(self.root, i) for i in range(13))

    def z2goto(self, a, b, c, d, e, f, g):
        """Go to a sideview area in Zelda 2"""
        mem = self.root.nes.mem
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

    def MenuBar(self):
        """Hook into the menubar and some menus."""
        if bimpy.begin_menu("Goto"):
            if bimpy.menu_item("Palace 1"):
                self.z2goto(4, 0, 3, 4, 0, 52, 0);                               
            elif bimpy.menu_item("Palace 2"):
                self.z2goto(4, 0, 3, 4, 1, 53, 0xe);                             
            elif bimpy.menu_item("Palace 3"):
                self.z2goto(4, 0, 4, 5, 2, 54, 0);                               
            elif bimpy.menu_item("Palace 4"):
                self.z2goto(4, 1, 4, 8, 0, 52, 0xf);                             
            elif bimpy.menu_item("Palace 5"):
                self.z2goto(4, 2, 3, 8, 0, 52, 0x23);                            
            elif bimpy.menu_item("Palace 6"):
                self.z2goto(4, 2, 4, 8, 1, 53, 0x24);                            
            elif bimpy.menu_item("Palace 7"):
                self.z2goto(5, 2, 5, 9, 2, 54, 0);                               
            elif bimpy.menu_item("Rauru"):
                self.z2goto(3, 0, 1, 0, 0xf8, 45, 2);                            
            elif bimpy.menu_item("Ruto"):
                self.z2goto(3, 0, 1, 1, 0xf8, 47, 5);                            
            elif bimpy.menu_item("Saria"):
                self.z2goto(3, 0, 1, 2, 0xf8, 49, 8);                            
            elif bimpy.menu_item("Mido"):
                self.z2goto(3, 0, 1, 3, 0xf8, 51, 11);                           
            elif bimpy.menu_item("Nabooru"):
                self.z2goto(3, 2, 2, 4, 0xf8, 45, 14);                           
            elif bimpy.menu_item("Darunia"):
                self.z2goto(3, 2, 2, 5, 0xf8, 47, 17);                           
            elif bimpy.menu_item("New Kasuto"):
                self.z2goto(3, 2, 2, 6, 0xf8, 49, 20);                           
            elif bimpy.menu_item("Old Kasuto"):
                self.z2goto(3, 2, 2, 7, 0xf8, 51, 23); 
            bimpy.end_menu()
        if bimpy.begin_menu("Cheats"):
            if bimpy.menu_item("Cheats", selected=self.cheats.visible.value):
                self.cheats.visible.value = not self.cheats.visible.value
            if bimpy.menu_item("Disable Encounters", selected=self.disable_encounters.value):
                self.disable_encounters.value = not self.disable_encounters.value
                self.DisableEncounters(self.disable_encounters.value)
            if bimpy.menu_item("Show Hitboxes", selected=self.hitbox_visible.value):
                self.hitbox_visible.value = not self.hitbox_visible.value
            if bimpy.menu_item("Memory Values", selected=self.memory.visible.value):
                self.memory.visible.value = not self.memory.visible.value
            if bimpy.menu_item("RNG", selected=self.rng.visible.value):
                self.rng.visible.value = not self.rng.visible.value
            bimpy.end_menu()

    def EmulateFrame(self):
        """Called for every frame."""
        for h in self.hitbox:
            h.Update()
        # Be sure to call the superclass too.
        super().EmulateFrame()

    def DrawImage(self):
        """Called on each frame to draw on the NES image."""
        mem = self.root.nes.mem
        # Only draw the hitboxes in gamestate "b" (side-view)
        if self.hitbox_visible.value and mem[0x736] == 0x0b:
            for h in self.hitbox:
                h.DrawImage()

    def Draw(self):
        """Called on each frame to draw your own GUIs."""
        self.cheats.Draw()
        self.memory.Draw()
        self.rng.Draw()

# Set the emulation hook to our custom class
app.root().hook = Zelda2()
