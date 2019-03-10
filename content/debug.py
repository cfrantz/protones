######################################################################
#
# Define some functions that might be useful in the PythonConsole
#
######################################################################
import app

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
    print("pc=%04x accessed addr %04x val=%02x" % (cpu.pc, addr, val))
    return val

def PrintExecCb(cpu):
    print("pc=%04x a=%02x x=%02x y=%02x sp=1%02x" % (
        cpu.pc, cpu.a, cpu.x, cpu.y, cpu.sp))
    PrintStack(cpu.sp)

    return cpu.pc

def ReadWatch(addr, on=True):
    app.root().nes.cpu.SetReadCallback(addr, PrintMemCb if on else None)

def WriteWatch(addr, on=True):
    app.root().nes.cpu.SetWriteCallback(addr, PrintMemCb if on else None)

def ExecWatch(addr, on=True):
    app.root().nes.cpu.SetExecCallback(addr, PrintExecCb if on else None)

