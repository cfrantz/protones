######################################################################
# Implement RNG display for Zelda2
######################################################################
import bimpy

def _bin(v):
    result = bin(v)[2:]
    return ('0' * (8-len(result))) + result

class RNG(object):
    WHITE = bimpy.Vec4(1.0, 1.0, 1.0, 1.0)
    RED = bimpy.Vec4(1.0, 0.0, 0.0, 1.0)
    GREEN = bimpy.Vec4(0.0, 1.0, 0.0, 1.0)

    def __init__(self, root):
        self.root = root
        self.visible = bimpy.Bool()
        self.rng_reset = -1
        self.root.nes.cpu.SetExecCallback(0x8d5e, self.RngReset)

    def RngReset(self, cpu):
        self.rng_reset = self.root.nes.mem[0x12]
        return cpu.pc

    def Draw(self):
        """Draw our RNG dialog box."""
        if not self.visible.value:
            return

        mem = self.root.nes.mem
        frame = mem[0x12]
        bimpy.begin('Zelda2 RNG')
        rng = [mem[0x51A + i] for i in range(9)]
        txt = ' '.join(_bin(v) for v in rng)

        bimpy.text_colored(self.GREEN if frame < 64 else self.WHITE,
                           'Frame = %03d' % frame)
        bimpy.text('Address:  $51A       +1       +2       +3       +4       +5       +6       +7       +8')
        if self.rng_reset == frame:
            bimpy.text('RNG ='); bimpy.same_line()
            bimpy.text_colored(self.RED, txt[:8]); bimpy.same_line()
            bimpy.text(txt[9:])
        else:
            bimpy.text('RNG = %s' % txt)
            self.rng_reset = -1

        nextbit = ((rng[0] ^ rng[1]) & 2) >> 1
        bimpy.text('      ^     |        |')
        bimpy.text('Next: %d-----+--------+' % nextbit)

        bimpy.end()
