import app
import bimpy
import code
import protones
import pydoc
import sys
import threading

pydoc.pager = pydoc.plainpager
sys.stdout.orig_write = sys.stdout.write
sys.stdout.orig_write = sys.stdout.write

class PythonConsole(code.InteractiveInterpreter):
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
    def __init__(self, root=None):
        self.root = root or app.root()

    def EmulateFrame(self):
        return self.root.nes.EmulateFrame()

    def GetPythonConsole(self):
        return PythonConsole(globals())

    def FileMenu(self):
        pass
    def EditMenu(self):
        pass
    def ViewMenu(self):
        pass
    def HelpMenu(self):
        pass
    def MenuBar(self):
        pass
    def DrawImage(self):
        pass
    def Draw(self):
        pass

app.EmulatorHooks = EmulatorHooks
app.root().hook = EmulatorHooks()

_addr = 0x8000
_length = 64
_bank = 0

def db(addr=None, length=None, b=None):
    global _addr, _length, _bank
    if addr is not None:
        _addr = addr
    if length is not None:
        _length = length
    if b is not None:
        _bank = b

    mem = app.root().nes.mem

    i = 0
    while i < _length:
        if i % 16 == 0:
            if i == 0:
                print('%04x:' % _addr, end='')
            else:
                print('  %s\n%04x:' % (''.join(buf), _addr), end='')
            buf = ['.' for _ in range(16)]
        if (_addr < 0x8000):
            val = mem[_addr]
        else:
            val = app.root().nes.cartridge.ReadPrg(
                    (0x4000 * _bank) + (_addr & 0x3FFF))
        print(' %02x' % val, end='')
        buf[i % 16] = chr(val) if val>=32 and val<127 else '.'
        _addr += 1
        i += 1

    if i % 16:
        i = 3 * (16 - i%16)
    else:
        i = 0
    print(' %*c%s' % (i, ' ', ''.join(buf)))
