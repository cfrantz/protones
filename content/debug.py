######################################################################
#
# Define some functions that might be useful in the PythonConsole
#
######################################################################
import app
import json

_addr = 0x8000
_length = 64
_bank = 0

def ReadPrgBank(bank, addr, length=None):
    if length is None:
        return app.root().nes.cartridge.ReadPrg((0x4000 * bank) + (addr & 0x3FFF))
    else:
        return [
            app.root().nes.cartridge.ReadPrg(
                (0x4000 * bank) + ((addr+i) & 0x3FFF)) for i in range(length)]

def WritePrgBank(bank, addr, val):
    if not isinstance(val, (list, tuple)):
        val = [val]
    for i, v in enumerate(val):
        app.root().nes.cartridge.WritePrg(
                (0x4000 * bank) + ((addr+i) & 0x3FFF), v)

def db(addr=None, length=None, b=None, ppu=False):
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
        if (ppu):
            val = mem.PPURead(_addr)
        elif (_addr < 0x8000):
            val = mem[_addr]
        else:
            val = ReadPrgBank(_bank, _addr)
        print(' %02x' % val, end='')
        buf[i % 16] = chr(val) if val>=32 and val<127 else '.'
        _addr += 1
        i += 1

    if i % 16:
        i = 3 * (16 - i%16)
    else:
        i = 0
    print(' %*c%s' % (i, ' ', ''.join(buf)))

def PrintStack(sp):
    sp += 0x100
    osp = sp
    mem = app.root().nes.mem
    s = []
    while True:
        sp += 1
        if sp == 0x200:
            break;
        addr = mem.ReadWord(sp, sp+1) - 2
        if addr >= 0x8000 and mem[addr] == 0x20:
            s.append('%04x' % addr)
            sp += 1
            if sp == 0x200:
                break;
        else:
            s.append('%02x' % mem[sp])

    print('SP=%04x: %s' % (osp, ' '.join(s)))


def PrintMemCb(cpu, addr, val):
    print("frame=%6d pc=%04x accessed addr %04x val=%02x" % (
        app.root().nes.frame(), cpu.pc, addr, val))
    return val

def PrintExecCb(cpu):
    print("pc=%04x a=%02x x=%02x y=%02x sp=1%02x" % (
        cpu.pc, cpu.a, cpu.x, cpu.y, cpu.sp))
    PrintStack(cpu.sp)

    return cpu.pc

def ReadWatch(addr, on=True):
    app.root().nes.cpu.SetReadCallback(addr, PrintMemCb if on else None)

def ReadWatchRange(start, end, on=True):
    for a in range(start, end):
        ReadWatch(a, on)

def WriteWatch(addr, on=True):
    app.root().nes.cpu.SetWriteCallback(addr, PrintMemCb if on else None)

def ExecWatch(addr, on=True):
    app.root().nes.cpu.SetExecCallback(addr, PrintExecCb if on else None)

def ExecDebugDot(addr, color=None):
    def _debug_dot(cpu):
        app.root().nes.SetDebugDot(color)
        return cpu.pc
    app.root().nes.cpu.SetExecCallback(addr, None if color is None else _debug_dot)

def ExecCycleCounter(a1=0, a2=0, n=0):
    nes = app.root().nes
    def _start(cpu):
        nes.mem[0x4100 + n*2] = 1
        return cpu.pc
    def _end(cpu):
        nes.mem[0x4100 + n*2 + 1] = 1
        return cpu.pc
    nes.cpu.SetExecCallback(a1, _start if a1 else None)
    nes.cpu.SetExecCallback(a2, _end if a1 else None)

def WatchAPU(on=True):
    for reg in range(0x4000, 0x4014):
        WriteWatch(reg, on)

class APULogger(object):

    def __init__(self):
        self.frame = {}
        self.first = 0
        self.reg = 0
        self.val = 0
        for reg in range(0x4000, 0x4014):
            app.root().nes.cpu.SetWriteCallback(reg, self.watcher)
        mem = app.root().nes.mem
        mem[0xeb] = 0x80

    def watcher(self, cpu, addr, val):
        frame = app.root().nes.frame()
        if self.first == 0:
            self.first = frame

        frame -= self.first
        if frame not in self.frame:
            self.frame[frame] = {"frame": frame}

        self.frame[frame][addr - 0x4000] = val
        return val

    def trigger(self, reg, val):
        mem = app.root().nes.mem
        self.reg = reg
        self.val = val
        self.frame = {}
        self.first = 0
        mem[reg] = val

    def save(self):
        fn = 'effect_%02X%02X.json' % (self.reg, self.val)
        with open(fn, 'wt') as f:
            f.write(json.dumps(list(self.frame.values()), indent=4))

class APUPlayer(object):

    def __init__(self):
        self.frame = {}
        self.first = 0
        app.root().hook.SetFrameCallback(self.callback)
        self.sweep = True

    def callback(self):
        frame = app.root().nes.frame()
        if frame in self.frame:
            f = self.frame[frame]
            mem = app.root().nes.mem
            for (k, v) in f.items():
                try:
                    k = int(k)
                    if not self.sweep and k in (1,5):
                        continue
                    mem[0x4000+k] = v
                except:
                    pass

    def play(self, fn):
        frame = app.root().nes.frame() + 1
        self.frame = {}
        with open(fn, 'rt') as f:
            data = json.load(f)
            for d in data:
                self.frame[frame+d['frame']] = d

