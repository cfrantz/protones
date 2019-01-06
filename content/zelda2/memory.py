######################################################################
# Implement Memory Display
######################################################################
import bimpy

def _bin(v):
    result = bin(v)[2:]
    return ('0' * (8-len(result))) + result

class Memory(object):
    WHITE = bimpy.Vec4(1.0, 1.0, 1.0, 1.0)
    RED = bimpy.Vec4(1.0, 0.0, 0.0, 1.0)
    GREEN = bimpy.Vec4(0.0, 1.0, 0.0, 1.0)

    def __init__(self, root):
        self.root = root
        self.visible = bimpy.Bool()

    def Draw(self):
        """Draw our Memory dialog box."""
        if not self.visible.value:
            return

        mem = self.root.nes.mem
        frame = mem[0x12]
        bimpy.begin('Zelda2 Memory Values')

        bimpy.text('Frame = %03d (0x%02x)' % (frame, frame))
        bimpy.separator()
        labels = ['Link', 'Enemy1', 'Enemy2', 'Enemy3', 'Enemy4', 'Enemy5', 'Enemy6']

        bimpy.text('            Xpos  Ypos  VelX  Anim  HP  ID  AI  It')
        bimpy.separator()
        for i, label in enumerate(labels):
            if i == 0:
                exists = 1
                hp = mem[0x774]
                eid = 0
                ai = 0
                it = 0
            else:
                exists = mem[0xb5 + i]
                hp = mem[0xc1 + i]
                eid = mem[0xa0 + i]
                ai = mem[0xae + i]
                it = mem[0x48d + i]

            velx = mem.ReadSignedByte(0x70 + i)
            page = mem[0x3b + i]
            posx = mem[0x4d + i]
            subpx = mem[0x3d6 + i]
            posy = mem[0x29 + i]
            anim = mem[0x80 + i]


            bimpy.text_colored(self.WHITE if exists else self.RED,
               '%6s: %2x.%02x.%02x    %02x  %4x  %4x  %02x  %02x  %02x  %02x' % (
                label, page, posx, subpx, posy, velx, anim, hp, eid, ai, it))

        bimpy.end()
