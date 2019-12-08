######################################################################
# Monitor for Arkista's Ring
# Trying to find the reason for the everdrive lockup.
#
# This script serves as an example of how to hook into ProtoNES 
# with python.
######################################################################
import app
import bimpy

# Subclass the EmulatorHooks class
class Arkista(app.EmulatorHooks):

    def __init__(self, root=None):
        # Call the superclass' init first.
        super().__init__(root)
        self.logging = False
        for addr in range(65536):
            self.root.nes.cpu.SetWriteCallback(addr, self.LogWrites)
        self.logfile = open("arkista.txt", "w")

    def LogWrites(self, cpu, addr, val):
        if self.logging:
            print("Write %04x = %02x" % (addr, val), file=self.logfile)
        return val;

    def MenuBar(self):
        """Hook into the menubar and some menus."""
        if bimpy.begin_menu("Arkista"):
            if bimpy.menu_item("Logging"):
                self.logging = not self.logging
                self.logfile.flush()

            bimpy.end_menu()

    def EmulateFrame(self):
        """Called for every frame."""
        return super().EmulateFrame()


# Set the emulation hook to our custom class
app.root().hook = Arkista()
