######################################################################
#
# Set up the python environment for ProtoNES.
#
######################################################################
import app
import bimpy
import code
import protones
import pydoc
import sys
import threading

from .debug import *

pydoc.pager = pydoc.plainpager
sys.stdout.orig_write = sys.stdout.write
sys.stdout.orig_write = sys.stdout.write

class PythonConsole(code.InteractiveInterpreter):
    """Create an Interpreter that the GUI's PythonConsole can use.

    This object captures stdout and stderr write functions so the
    data can be written into the PythonConsole GUI element.
    """
    def __init__(self, *args, **kwargs):
        code.InteractiveInterpreter.__init__(self, *args, **kwargs)
        self.outbuf = ''
        self.errbuf = ''
        sys.stdout.write = self.outwrite
        sys.stderr.write = self.errwrite

    def outwrite(self, data):
        self.outbuf += data

    def errwrite(self, data):
        self.errbuf += data

    def GetOut(self):
        data = self.outbuf
        self.outbuf = ''
        return data;

    def GetErr(self):
        data = self.errbuf
        self.errbuf = ''
        return data;

class EmulatorHooks(object):
    """EmulatorHooks defines the hooks used to extend the application.

    You can subclass this class to add functionality to the emulator.
    """

    def __init__(self, root=None):
        """Get the root ProtoNES object."""
        self.root = root or app.root()
        self.framecb = None

    def SetFrameCallback(self, cb=None):
        self.framecb = cb

    def EmulateFrame(self):
        """Emulate a single frame."""
        if self.framecb is not None:
            self.framecb()
        return self.root.nes.EmulateFrame()

    def GetPythonConsole(self):
        """Get a PythonConsole interpreter."""
        return PythonConsole(globals())

    def FileMenu(self):
        """Called when creating the File menu."""
        pass
    def EditMenu(self):
        """Called when creating the Edit menu."""
        pass
    def ViewMenu(self):
        """Called when creating the View menu."""
        pass
    def HelpMenu(self):
        """Called when creating the Help menu."""
        pass
    def MenuBar(self):
        """Called when creating the Menu bar, after 'Help'."""
        pass
    def DrawImage(self):
        """Called when drawing the current PPU image to the screen."""
        pass
    def Draw(self):
        """Called when drawing the GUI."""
        pass

app.EmulatorHooks = EmulatorHooks
app.root().hook = EmulatorHooks()
