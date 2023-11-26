######################################################################
# Implement RNG display for Zelda2
######################################################################
from application import gui

def _bin(v):
    result = bin(v)[2:]
    return ('0' * (8-len(result))) + result

class RNG(object):
    WHITE = gui.Vec4(1.0, 1.0, 1.0, 1.0)
    RED = gui.Vec4(1.0, 0.0, 0.0, 1.0)
    GREEN = gui.Vec4(0.0, 1.0, 0.0, 1.0)

    def __init__(self, app):
        self.app = app
        self.visible = False
        self.reset = -1
        self.app.nes.cpu.set_exec_callback(0x8d5e, self.rng_reset)

    def rng_reset(self, cpu):
        self.reset = self.app.nes.mem[0x12]
        return cpu.pc

    def draw(self):
        """Draw our RNG dialog box."""
        if not self.visible:
            return

        mem = self.app.nes.mem
        frame = mem[0x12]
        gui.begin('Zelda2 RNG')
        rng = [mem[0x51A + i] for i in range(9)]
        txt = ' '.join(_bin(v) for v in rng)

        gui.text_colored(self.GREEN if frame < 64 else self.WHITE,
                           'Frame = %03d' % frame)
        gui.text('Address:  $51A       +1       +2       +3       +4       +5       +6       +7       +8')
        if self.reset == frame:
            gui.text('RNG ='); gui.same_line()
            gui.text_colored(self.RED, txt[:8]); gui.same_line()
            gui.text(txt[9:])
        else:
            gui.text('RNG = %s' % txt)
            self.reset = -1

        nextbit = ((rng[0] ^ rng[1]) & 2) >> 1
        gui.text('      ^     |        |')
        gui.text('Next: %d-----+--------+' % nextbit)

        gui.end()
