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
